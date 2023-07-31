#include "ICMP.h"
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <err.h>
#include <unistd.h>
#include <netdb.h>
#include <memory.h>

#define SIZE_ICMP_HDR   8

extern struct timeval theCurrentTime;

#include <stdio.h>

ICMP::ICMP(int aDataLen)
{
    struct protoent *proto;
    theDataLen = aDataLen;
    theSendSize = theDataLen + SIZE_ICMP_HDR;
    theIdent = getpid() & 0xffff;
    if ((proto = getprotobyname("icmp")) == 0)
	errx(1, "unknown protocol: icmp");
    if ((theSock = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0)
	err(1, "SOCK_RAW");
}

ICMP::~ICMP()
{
	close(theSock);
}

int
ICMP::in_cksum(unsigned short *ptr, int size) const
{

    long sum;
    unsigned short oddbyte;
    unsigned short answer;

    sum = 0;
    while (size > 1) {
	sum += *ptr++;
	size -= 2;
    }

    if (size == 1) {
	oddbyte = 0;
	*((unsigned char *)&oddbyte) = *(unsigned char *)ptr;
	sum += oddbyte;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}

#ifndef ICMP_MINLEN
#define ICMP_MINLEN  8
#endif

int
ICMP::recv(struct sockaddr* from, socklen_t& fromlen, unsigned int& seq, struct timeval& sent_tv, struct timeval& recv_tv, uint8_t& ttl) const
{
    static int n;

    int iphdrlen;
    struct ip *ip = 0;
    struct icmp *icp = 0;

    n = recvfrom(theSock, (void*)theRecvPkt, sizeof(theRecvPkt), 0, from, &fromlen);
    gettimeofday(&theCurrentTime, 0);
    recv_tv = theCurrentTime;

    ip = (struct ip *)theRecvPkt;
    iphdrlen = ip->ip_hl << 2;

    if (n < iphdrlen + ICMP_MINLEN)
	return 0;       // pkt too short
    n -= iphdrlen;
    icp = (struct icmp *)(theRecvPkt + iphdrlen);
    if (icp->icmp_type != ICMP_ECHOREPLY)
	return 0;
    if (icp->icmp_id != theIdent)
	return 0;

#ifndef icmp_data
    sent_tv = *(struct timeval *)(icp + 1);
#else
    sent_tv = *(struct timeval *)&icp->icmp_data[0];
#endif
    seq = ntohs(icp->icmp_seq);
    ttl = ip->ip_ttl;
    return n;
}

void
ICMP::send(unsigned int seq, struct sockaddr *to, socklen_t tolen) const
{
    struct icmp *icp;
    memset((void**)theSendPkt, '\0', theSendSize);
    icp = (struct icmp *)theSendPkt;
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_id = theIdent;
    icp->icmp_seq = htons(seq);
    gettimeofday(&theCurrentTime, 0);
    *(struct timeval *)(theSendPkt + SIZE_ICMP_HDR) = theCurrentTime;
    icp->icmp_cksum = in_cksum((unsigned short *)icp, theSendSize);
    sendto(theSock, theSendPkt, theSendSize, 0, to, tolen);
}

int
ICMP::select(int wait) const
{
    fd_set R;
    FD_ZERO(&R);
    FD_SET(theSock, &R);
    struct timeval to;
    to.tv_sec = 0;
    to.tv_usec = wait;
    int x = ::select(theSock+1, &R, 0, 0, &to);
    return x;
}

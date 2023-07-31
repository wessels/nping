
#include "site.h"
#include "misc.h"
#include <stdlib.h>
#include <netinet/in.h>
#include <err.h>
#include <memory.h>
#include <netdb.h>

#define MAX_N_AVG 12

extern struct timeval theCurrentTime;

site::site(const char *host_or_addr, struct timeval nextPingTime)
{
	if (1 != theAddr.pton(host_or_addr))
		errx(1, "unknown host '%s'", host_or_addr);
	theRTTs = (double *) calloc(HISTSIZE, sizeof(*theRTTs));
	theTTLs = (uint8_t *) calloc(HISTSIZE, sizeof(*theTTLs));
	for (unsigned int j=0; j<HISTSIZE; j++)
		theRTTs[j] = -1.0;
	thePtr = "";
	theWeightedAvgRTT = 0.0;
	theHighSeq = 0;
	nPktsSent = 0;
	nPktsRecv = 0;
	theNextPingTime = nextPingTime;
	next = 0;
}

site::~site()
{
	free(theRTTs);
	free(theTTLs);
}

void
site::recv(unsigned int seq, struct timeval sent, struct timeval rcvd, uint8_t ttl)
{
    double rtt = (double)tvsub(&sent, &rcvd) / 1000;
    if (rtt < 1.0)
	rtt = 1.0;

    unsigned int N = ++nPktsRecv;
    if (N >= MAX_N_AVG)
	N = MAX_N_AVG;
    theWeightedAvgRTT = ((theWeightedAvgRTT * (N - 1)) + rtt) / N;

    if (seq > theHighSeq) {
	for (unsigned int j = 0; j < HISTSIZE; j++) {
	    unsigned int k = j + seq - theHighSeq;
	    theRTTs[j] = k < HISTSIZE ? theRTTs[k] : 0;
	    theTTLs[j] = k < HISTSIZE ? theTTLs[k] : 0;
	}
	theHighSeq = seq;
    }
    theRTTs[HISTSIZE - 1] = rtt;
    theTTLs[HISTSIZE - 1] = ttl;
    stats();
}

void
site::stats()
{
	double _sum = 0;
	unsigned int _nloss = 0;
	unsigned int _nsent = 0;
	unsigned int _count = 0;
	theMaxRTT = 0.0;
	theMinRTT = 999999999.0;
	for (unsigned int j = 0; j < HISTSIZE; j++) {
	    if (theRTTs[j] < 0.0)
		continue;
	    _nsent++;
	    if (theRTTs[j] == 0.0) {
		_nloss++;
		continue;
	    }
	    if (theRTTs[j] < theMinRTT)
		theMinRTT = theRTTs[j];
	    if (theRTTs[j] > theMaxRTT)
		theMaxRTT = theRTTs[j];
	    _sum += theRTTs[j];
	    _count++;
	}
	thePktLoss = _nsent ? 100.0 * _nloss / _nsent : 0.0;
	theMeanRTT = _count ? _sum / _count : 0.0;
	if (999999999.0 == theMinRTT)
		theMinRTT = 0.0;
}

void
site::sent(unsigned int seq, struct timeval add)
{
    struct timeval result;
    timeradd(&theNextPingTime, &add, &result);
    theNextPingTime = result;
    if (nPktsSent > theHighSeq) {
	for (unsigned int j = 0; j < HISTSIZE; j++) {
	    unsigned int k = j + nPktsSent - theHighSeq;
	    theRTTs[j] = k < HISTSIZE ? theRTTs[k] : 0;
	}
	theHighSeq = nPktsSent;
    }
    nPktsSent++;
}

unsigned int
site::next_seq()
{
    return nPktsSent+1;
}

bool
site::ttp() const
{
	return timercmp(&theCurrentTime, &theNextPingTime, >) ? true : false;
}

struct sockaddr *
site::sockaddr(socklen_t *len) const
{
    static struct sockaddr_storage ss;
    memset(&ss, sizeof(ss), 0);
    if (4 == theAddr.version()) {
	struct sockaddr_in *sin = (struct sockaddr_in *) &ss;
	sin->sin_family = AF_INET;
	sin->sin_addr = theAddr.v4();
	if (len) *len = sizeof(struct sockaddr_in);
#ifndef __linux__
	sin->sin_len = 4;
#endif
    } else if (6 == theAddr.version()) {
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) &ss;
	sin6->sin6_family = AF_INET6;
	sin6->sin6_addr = theAddr.v6();
	if (len) *len = sizeof(struct sockaddr_in6);
#ifndef __linux__
	sin6->sin6_len = 16;
#endif
    }
    return (struct sockaddr *) &ss;
}

const char *
site::ptr()
{
	if (thePtr.length())
		return thePtr.c_str();
	char buf[128];
	socklen_t salen;
	struct sockaddr *sa = sockaddr(&salen);
	if (0 != getnameinfo(sa, salen, buf, sizeof(buf), NULL, 0, NI_NAMEREQD))
		addr().ntop(buf, sizeof(buf));
	thePtr = buf;
	return thePtr.c_str();
}

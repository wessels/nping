#ifndef _ICMP_H_
#define _ICMP_H_

#include <sys/socket.h>
#include <inttypes.h>

#define MAX_ICMP_PKT_SZ 4096

class ICMP {
    public:
	ICMP(int aDataSize = 56);
	~ICMP();
	int select(int usec) const;
	int in_cksum(unsigned short *ptr, int size) const;
	int recv(struct sockaddr* from, socklen_t& fromlen, unsigned int& seq, struct timeval& sent_tv, struct timeval& recv_tv, uint8_t& ttl) const;
	void send(unsigned int seq, struct sockaddr *to, socklen_t tolen) const;
    private:
	int theIdent;
	int theSock;
	int theDataLen;
	char theSendPkt[MAX_ICMP_PKT_SZ];
	char theRecvPkt[MAX_ICMP_PKT_SZ];
	size_t theSendSize;
};

#endif

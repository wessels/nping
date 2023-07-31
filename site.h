#ifndef _SITE_H_
#define _SITE_H_

#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

#include "inX_addr.h"

class site {
    public:
	site(const char *host_or_addr, struct timeval nextPingTime);
	~site();
	// pinging
	unsigned int next_seq();
	void sent(unsigned int seq, struct timeval add);
	void recv(unsigned int seq, struct timeval sent, struct timeval rcvd, uint8_t ttl);
	bool ttp() const;	// time to ping?
	struct sockaddr *sockaddr(socklen_t*) const;
	// stats
	double max_rtt() const { return theMaxRTT; };
	double min_rtt() const { return theMinRTT; };
	double avg_rtt() const { return theMeanRTT; };
	double wav_rtt() const { return theWeightedAvgRTT; };
	double pkt_loss() const { return thePktLoss; };
	double nrecv() const { return nPktsRecv; };
	double rtt(unsigned int j) const { return theRTTs[j]; };
	// other
	INX_ADDR addr() const { return theAddr; };
	const char * ptr();
	const site *next;
	static const unsigned int HISTSIZE = 200;
    private:
	INX_ADDR theAddr;
	std::string thePtr;
	double *theRTTs;
	uint8_t *theTTLs;
	double theWeightedAvgRTT;
	unsigned int theHighSeq;
	unsigned int nPktsSent;
	unsigned int nPktsRecv;		  /* total # pings sent */
	struct timeval theNextPingTime;
	double theMinRTT;
	double theMaxRTT;
	double theMeanRTT;
	double thePktLoss;
	void stats();
};

#endif

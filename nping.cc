#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <err.h>
#include <string.h>

#include "ICMP.h"
#include "site.h"
#include "renderer.h"

struct timeval thePeriod = {10,0};
struct timeval theCurrentTime;

static site *
find_a_site_to_ping(const site *sites)
{
    for (const site *s = sites; s; s=s->next) {
	if (s->ttp())
		return (site*) s;
    }
    return 0;
}

static site *
find_site_by_addr(const site *sites, struct sockaddr *from)
{
    struct sockaddr_in *sin = (struct sockaddr_in *) from;
    for (const site *s = sites; s; s=s->next) {
	const struct sockaddr_in *x = (struct sockaddr_in *) s->sockaddr(NULL);
	if (x->sin_addr.s_addr == sin->sin_addr.s_addr)
		return (site*) s;
    }
    return 0;
}

static unsigned int
count_sites(char *argv[])
{
	int count = 0;
	for (unsigned int i=0; argv[i]; i++) {
		char *q = strchr(argv[i], '/');
		if (0 == q) {
			count++;
		} else {
			unsigned int preflen = atoi(q+1);
			if (preflen > 32)
				errx(1, "bad prefix length on '%s'", argv[i]);
			unsigned int naddrs = 1U << (32-preflen);
			count += naddrs - 2;
		}
	}
	return count;
}

struct timeval
site_ping_time(unsigned int k, unsigned int c)
{
	struct timeval result;
	struct timeval add;
	double x = (double) (thePeriod.tv_sec + (0.0000001 * thePeriod.tv_usec)) * k / c;
	add.tv_sec = (int) x;
	add.tv_usec = (x - add.tv_sec) * 1000000;
	timeradd(&theCurrentTime, &add, &result);
	return result;
}

static const site *
make_sites(char *argv[]) {
	unsigned int count = count_sites(argv);
	unsigned int sn = 0;
	const site *h = NULL;
	const site **t = &h;
	gettimeofday(&theCurrentTime, NULL);
	for (unsigned int i=0; argv[i]; i++) {
		site *s;
		char *q = strchr(argv[i], '/');
		fprintf(stderr, "creating sites from %s\n", argv[i]);
		if (NULL != q) {
			*q = '\0';
			unsigned int a;
			if (1 != inet_pton(AF_INET, argv[i], &a))
				errx(1, "Invalid IP '%s'", argv[i]);
			unsigned int preflen = atoi(q+1);
			*q = '/';
			if (preflen > 32)
				errx(1, "bad prefix length on '%s'", argv[i]);
			unsigned int naddrs = 1U << (32-preflen);
			a = ntohl(a);
			for (unsigned int j = 1; j<(naddrs-1); j++) {
				char buf[128];
				unsigned int k = htonl(a+j);
				inet_ntop(AF_INET, &k, buf, sizeof(buf));
				s = new site(buf, site_ping_time(sn++,count));
				*t = s;
				t = &s->next;
			}
		} else {
			s = new site(argv[i], site_ping_time(sn++,count));
			*t = s;
			t = &s->next;
		}
	}
	return h;
}

char *
ntop(struct in_addr a)
{
	static char buf[128];
	inet_ntop(AF_INET, &a, buf, sizeof(buf));
	return buf;
}

static void
loop(const site *sites, const ICMP *ping, const renderer *rend)
{
    struct sockaddr *to;
    socklen_t fromlen = sizeof(struct sockaddr_storage);
    socklen_t tolen = 0;
    struct sockaddr *from = (struct sockaddr*) malloc(fromlen);
    while (1) {
	site *s;
	int x = ping->select(100);
	if (x > 0) {
		uint8_t ttl;
		unsigned int seq;
		struct timeval sent_tv;
		struct timeval recv_tv;
		if (ping->recv(from, fromlen, seq, sent_tv, recv_tv, ttl))
		    if ((s = find_site_by_addr(sites, from)))
			s->recv(seq, sent_tv, recv_tv, ttl);
	}
	gettimeofday(&theCurrentTime, NULL);
	if (x == 0 && (s = find_a_site_to_ping(sites))) {
		to = s->sockaddr(&tolen);
		ping->send(s->next_seq(), to, tolen);
		s->sent(s->next_seq(), thePeriod);
	}
	rend->render(sites);
    }
}

void
usage(const char *me)
{
	fprintf(stderr, "usage: %s [-p period] CIDR\n", me);
	exit(1);
}

int
main(int argc, char *argv[])
{
    int c;
    const char *progname = argv[0];
    while ((c = getopt(argc, argv, "p:")) != -1) {
	switch (c) {
	case 'p':
	    thePeriod.tv_sec = (unsigned int) strtod(optarg, NULL);
	    thePeriod.tv_usec = (strtod(optarg, NULL) - thePeriod.tv_sec) * 1000000;
	    break;
	default:
	    usage(progname);
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if (0 == argc)
	usage(progname);

    const site *sites = make_sites(argv);
    ICMP *ping = new ICMP();
    renderer *rend = new renderer();
    loop(sites, ping, rend);
    delete rend;
    delete ping;
}

int
tvsub(struct timeval *t1, struct timeval *t2)
{
    return (t2->tv_sec - t1->tv_sec) * 1000000 + (t2->tv_usec - t1->tv_usec);
}


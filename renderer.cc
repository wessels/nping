#include "renderer.h"
#include "site.h"
#include <string.h>

extern struct timeval theCurrentTime;
extern char *ntop(struct in_addr);

renderer::renderer()
{
	theWindow = initscr();
}

renderer::~renderer()
{
	endwin();
}

void
renderer::render(const site *sites) const
{
    move(0, 0);
    for (const site *s = sites; s ; s = s->next) {
	if (0 == s->nrecv())
		continue;
	const char *ptr = ((site*) s)->ptr();
	char ipaddr[128];
	char buf[128];
	if (0 == ptr) {
	    s->addr().ntop(ipaddr, sizeof(ipaddr)),
	    ptr = ipaddr;
	}
	snprintf(buf, sizeof(buf), "%-40.40s %d/%d/%d, %dms, %d%% loss",
	    ptr,
	    (int) (s->min_rtt()+0.5),
	    (int) (s->avg_rtt()+0.5),
	    (int) (s->max_rtt()+0.5),
	    (int) (s->wav_rtt()+0.5),
	    (int) (s->pkt_loss()+0.5));
	printw("%-69.69s", buf);
	int xtracols = getmaxx(theWindow) - 70;
	for (int j = (s->HISTSIZE - xtracols); j < (int) s->HISTSIZE; j++) {
	    if (j < 0 || j >= (int) s->HISTSIZE)
		continue;
	    if (s->rtt(j) < 0.0)
		addch(' ');
	    else if (s->rtt(j) == 0.0)
		addch('.');
	    else
		addch('!');
	}
	addch('\n');
    }
    move(getmaxy(theWindow) - 1, 0);
    addstr(ctime(&theCurrentTime.tv_sec));
    refresh();
}

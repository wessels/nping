
OBJS=nping.o ICMP.o site.o renderer.o inX_addr.o
PROG=nping
CPPFLAGS=-g -Wall
LDFLAGS=-g

all:: ${PROG}

${PROG}: ${OBJS}
	${CXX} ${LDFLAGS} -o $@ ${OBJS} -lcurses

install:: ${PROG}
	install -m 4755 ${PROG} /usr/local/sbin/${PROG}


clean::
	rm -f ${OBJS}


#ifndef INX_ADDR_H
#define INX_ADDR_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ostream>

class INX_ADDR {
public:
	INX_ADDR();
	INX_ADDR(const char *);
	int version() const;
	const char *ntop(char *, socklen_t) const;
	int pton(const char *);
	unsigned int hash() const;
	struct in_addr v4() const;
	struct in6_addr v6() const;
	bool operator==(INX_ADDR) const;
	bool operator!=(INX_ADDR) const;
	bool operator<(INX_ADDR) const;
	INX_ADDR& operator=(struct in_addr);
	INX_ADDR& operator=(struct in6_addr);
	INX_ADDR mask(const INX_ADDR&);
	static int cmp(const INX_ADDR *, const INX_ADDR *);
	static unsigned int hash(const INX_ADDR&);

private:
	int is_v4_in_v6() const;
	union {
		struct in6_addr in6;
		struct {
			struct in_addr pad0;
			struct in_addr pad1;
			struct in_addr pad2;
			struct in_addr in4;
		} _;
	} theAddr;
};

extern std::ostream& operator<<(std::ostream& os, const INX_ADDR& addr);

#ifdef _HASH_MAP
namespace __gnu_cxx {
template<>  
struct hash<INX_ADDR> {
  size_t operator()(const INX_ADDR& that) const { return that.hash(); }
};
}
#endif

#endif /* INX_ADDR_H */

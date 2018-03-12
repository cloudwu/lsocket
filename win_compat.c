#define WIN_COMPAT_IMPL
#include "win_compat.h"
#include "iphlpapi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int 
win_getsockopt(SOCKET sockfd, int level, int optname, void *optval, socklen_t *optlen) {
	if (optname == SO_NOSIGPIPE) {
		// ignore
		return 0;
	}
	int size = (int)*optlen;
	int ret = getsockopt(sockfd, level, optname, (char *)optval, &size);
	if (ret == 0) {
		*optlen = size;
		return 0;
	} else {
		return -1;
	}
}

int 
win_setsockopt(SOCKET sockfd, int level, int optname, const void *optval, socklen_t optlen) {
	if (optname == SO_NOSIGPIPE) {
		// ignore
		return 0;
	}
	int ret = setsockopt(sockfd, level, optname, (const char *)optval, (int)optlen);
	if (ret == 0) {
		return 0;
	} else {
		return -1;
	}
}

int
fcntl(SOCKET fd, int cmd, int value) {
	unsigned long on = 1;
	return ioctlsocket(fd, FIONBIO, &on);
}

#define NS_INT16SZ   2
#define NS_IN6ADDRSZ  16

static const char *
inet_ntop4(const unsigned char *src, char *dst, size_t size) {
	char tmp[sizeof "255.255.255.255"];
	size_t len = snprintf(tmp, sizeof(tmp), "%u.%u.%u.%u", src[0], src[1], src[2], src[3]);
	if (len >= size) {
		return NULL;
	}
	memcpy(dst, tmp, len + 1);

	return dst;
}

static const char *
inet_ntop6(const unsigned char *src, char *dst, size_t size) {
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
	struct { int base, len; } best, cur;
	unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i, inc;

	memset(words, '\0', sizeof(words));
	for (i = 0; i < NS_IN6ADDRSZ; i++) {
		words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
	}
	best.base = -1;
	best.len = 0;
	cur.base = -1;
	cur.len = 0;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (words[i] == 0) {
			if (cur.base == -1) {
				cur.base = i, cur.len = 1;
			} else {
				cur.len++;
			}
		} else if (cur.base != -1) {
			if (best.base == -1 || cur.len > best.len) {
				best = cur;
			}
			cur.base = -1;
		}
	}
	if (cur.base != -1) {
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	tp = tmp;
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
		if (best.base != -1 && i >= best.base &&
			i < (best.base + best.len)) {
				if (i == best.base)
					*tp++ = ':';
			continue;
		}
		if (i != 0)
			*tp++ = ':';
		if (i == 6 && best.base == 0 &&
			(best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
			if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
				return NULL;
			tp += strlen(tp);
			break;
		}
		inc = snprintf(tp, 5, "%x", words[i]);
		tp += inc;
	}
	if (best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
		*tp++ = ':';
	*tp++ = '\0';

	if ((size_t)(tp - tmp) > size) {
		return NULL;
	}
	memcpy(dst, tmp, tp - tmp);
	return dst;
}

const char *
inet_ntop(int af, const void *src, char *dst,  socklen_t size) {
	switch (af) {
	case AF_INET:
		return inet_ntop4((const unsigned char*)src, dst, size);
	case AF_INET6:
		return inet_ntop6((const unsigned char*)src, dst, size);
	default:
		return NULL;
	}
}

void
init_socketlib(lua_State *L) {
	static int init = 0;
	if (init)
		return;
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (result != 0) {
		luaL_error(L, "WSAStartup failed: %d\n", result);
	}
	init = 1;
}

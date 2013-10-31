#include "my_socket.h"

int tcp_socket(void)
{
	return socket(AF_INET, SOCK_STREAM, 0);
}

int udp_socket(void)
{
	return socket(AF_INET, SOCK_DGRAM, 0);
}

int connect_tcp_timeout(int sockfd, char *ip_address, int port, int timeout)
{
	fprintf(stderr, "not yet implement\n");
	exit(1);
}

/* from kolc */
int connect_tcp(int sockfd, char *host, int port)
{
	struct sockaddr_in servaddr;
	struct sockaddr_in *resaddr;
	struct addrinfo    hints;
	struct addrinfo    *res;
	int err;

	res = 0;
	memset((char *)&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	if ( (err = getaddrinfo(host, 0, &hints, &res)) != 0) {
		return -1;
	}

	resaddr = (struct sockaddr_in *)res->ai_addr;
	memset((char *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(port);
	servaddr.sin_addr   = resaddr->sin_addr;
	freeaddrinfo(res);

	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect");
		return -1;
	}
	//return connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	return 0;
}

int connect_udp(int sockfd, char *host, int port)
{
	struct sockaddr_in servaddr;
	struct sockaddr_in *resaddr;
	struct addrinfo    hints;
	struct addrinfo    *res;
	int err;

	res = 0;
	memset((char *)&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	if ( (err = getaddrinfo(host, 0, &hints, &res)) != 0) {
		return -1;
	}

	resaddr = (struct sockaddr_in *)res->ai_addr;
	memset((char *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(port);
	servaddr.sin_addr   = resaddr->sin_addr;
	freeaddrinfo(res);

	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect");
		return -1;
	}
	//return connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	return 0;
}

int my_bind(int sockfd, char *host, int port)
{
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, host, &addr.sin_addr.s_addr);
    addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        warn("bind");
        return -1;
    }

    return 0;
}

int get_so_rcvbuf(int sockfd)
{
    int ret_so_rcvbuf;
    socklen_t len;
    
    len = sizeof(ret_so_rcvbuf);

    if(getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &ret_so_rcvbuf, &len) < 0) {
        warn("cannot get SO_RCVBUF");
        return -1;
    }

    return ret_so_rcvbuf;
}

int set_so_rcvbuf(int sockfd, int so_rcvbuf)
{
    socklen_t len;
    len = sizeof(so_rcvbuf);
    int ret_so_rcvbuf = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &so_rcvbuf, len) < 0) {
        warn("cannot set SO_RCVBUF");
        return -1;
    }

    if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &ret_so_rcvbuf, &len) < 0) {
        warn("cannot get SO_RCVBUF");
        return -1;
    }

    ret_so_rcvbuf = get_so_rcvbuf(sockfd);

    return ret_so_rcvbuf;
}

int get_so_sndbuf(int sockfd)
{
    int ret_so_sndbuf;
    socklen_t len;
    
    len = sizeof(ret_so_sndbuf);

    if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &ret_so_sndbuf, &len) < 0) {
        warn("cannot get SO_SNDBUF");
        return -1;
    }

    return ret_so_sndbuf;
}

int set_so_sndbuf(int sockfd, int so_sndbuf)
{
    socklen_t len;
    len = sizeof(so_sndbuf);
    int ret_so_sndbuf = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &so_sndbuf, len) < 0) {
        warn("cannot set SO_RCVBUF");
        return -1;
    }

    if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &ret_so_sndbuf, &len) < 0) {
        warn("cannot get SO_SNDBUF");
        return -1;
    }

    ret_so_sndbuf = get_so_sndbuf(sockfd);

    return ret_so_sndbuf;
}

int set_so_nodelay(int sockfd)
{
    int on = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY , &on, sizeof(on)) < 0) {
        warn("setsockopt nodelay");
        return -1;
    }

    return 0;
}

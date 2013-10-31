/* Taken from "The Practice of Programming, Kernighan and Pike" */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "host_info.h"

host_info *new_host(char *host_and_port)
{
	host_info *newp;
	char *tmp;

	if ( (newp = malloc(sizeof(host_info))) == NULL) {
		perror("malloc in new_host for host_info");
		return NULL;
	}
	memset(newp, 0, sizeof(host_info));
	tmp = strdup(host_and_port);
	newp->ip_address = strsep(&tmp, ":");
	if (tmp != NULL) {
		newp->port = atoi(tmp);
	}
	else {
		newp->port = DEFAULT_PORT;
	}
	newp->bufsize = DEFAULT_BUFSIZE;
	if ( (newp->buf = malloc(newp->bufsize)) == NULL) {
		perror("malloc_in new_host (for buf)");
		return NULL;
	}
	return newp;
}

host_info *addfront(host_info *host_list, host_info *newp)
{
	newp->next = host_list;
	return newp;
}

host_info *addend(host_info *host_list, host_info *newp)
{
	host_info *p;
	if (host_list == NULL) {
		return newp;
	}
	for (p = host_list; p->next != NULL; p = p->next) {
		;
	}
	p->next = newp;
	return host_list;
}

int connect_to_server(host_info *p, int timeout)
{
	struct sockaddr_in servaddr;
	struct timeval tm_out;
	
	if ( (p->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		err(1,"socket()");
	}
	// XXX: should verify port is valid, return value of inet_aton,
	//      use getaddrinfo() etc.
	//memset(&servaddr, 0, sizeof(servaddr));
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(p->port);
	if (inet_aton(p->ip_address,  &servaddr.sin_addr) == 0) {
		warnx("IP address invalid");
		exit(1);
	}
	tm_out.tv_sec = timeout;
	tm_out.tv_usec = 0;
	if (setsockopt(p->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tm_out, sizeof(tm_out)) < 0) {
		err(1, "socket timeout set");
	}
	if (connect(p->sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0) {
		err(1, "connect");
	}
	return 0;
}

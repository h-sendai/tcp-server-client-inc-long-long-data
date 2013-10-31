#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "my_socket.h"

int tcp_listen(int port)
{
    int on = 1;
    int listenfd;
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port        = htons(port);

    listenfd = tcp_socket();
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        warn("bind");
        return -1;
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        warn("setsockopt");
        return -1;
    }

    if (listen(listenfd, 10) < 0) {
        warn("listen");
        return -1;
    }
    
    return listenfd;
}

int accept_connection(int port)
{
    struct sockaddr_in remote_addr;
    socklen_t addr_len;

    int sockfd, listenfd;

    listenfd = tcp_listen(port);
    if (listenfd < 0) {
        fprintf(stderr, "tcp_listen");
        return -1;
    }
    
    sockfd = accept(listenfd, (struct sockaddr *) &remote_addr, &addr_len);
    if (sockfd < 0) {
        warn("accept");
        return -1;
    }

    return sockfd;
}

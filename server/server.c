#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "my_signal.h"
#include "my_socket.h"
#include "accept_connection.h"
#include "get_timestamp_us.h"

int debug = 0;

int child_proc(int connfd)
{
    int n;
    unsigned char buf[2*1024*1024];
    unsigned long long n_loop = 0;
    char timestamp[128];

    for ( ; ; ) {
        n = read(connfd, buf, sizeof(buf));
        if (n < 0) {
            err(1, "read");
        }
        else if (n == 0) {
            exit(0);
        }
        if (debug >= 1) {
            if (n_loop % 1000 == 0) {
                get_timestamp_us(timestamp, sizeof(timestamp));
                fprintf(stderr, "%s debug: %d bytes read\n", timestamp, n);
            }
            n_loop ++;
        }
    }
    return 0;
}

void sig_chld(int signo)
{
    pid_t pid;
    int   stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        ;
    }
    return;
}

int main(int argc, char *argv[])
{
    int port = 1234;
    pid_t pid;
    struct sockaddr_in remote;
    socklen_t addr_len;
    int listenfd;
    int c;

    while ( (c = getopt(argc, argv, "d")) != -1) {
        switch (c) {
            case 'd':
                debug += 1;
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    my_signal(SIGCHLD, sig_chld);
    my_signal(SIGPIPE, SIG_IGN);

    listenfd = tcp_listen(port);
    if (listenfd < 0) {
        errx(1, "tcp_listen");
    }

    for ( ; ; ) {
        int connfd = accept(listenfd, (struct sockaddr *)&remote, &addr_len);
        if (connfd < 0) {
            errx(1, "accept_connection");
        }
        
        pid = fork();
        if (pid == 0) { //child
            if (close(listenfd) < 0) {
                err(1, "close listenfd");
            }
            if (child_proc(connfd) < 0) {
                errx(1, "child_proc");
            }
            exit(0);
        }
        else { // parent
            if (close(connfd) < 0) {
                err(1, "close for accept socket of parent");
            }
        }
    }
        
    return 0;
}

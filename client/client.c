#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "get_num.h"
#include "host_info.h"
#include "my_signal.h"
#include "my_socket.h"
#include "readn.h"
#include "set_timer.h"

struct timeval begin, end;
unsigned long long so_far_bytes = 0;
int n_loop;
int bufsize;

int usage()
{
    fprintf(stderr, "Usage: client ip_address port bufsize_kB\n");
    return 0;
}

void print_result(int signo)
{
    struct timeval diff;
    double run_time;
    double tp;

    fprintf(stderr, "print_result\n");
    gettimeofday(&end, NULL);
    timersub(&end, &begin, &diff);
    run_time = diff.tv_sec + 0.000001*diff.tv_usec;
    tp = (double)so_far_bytes / run_time / 1024.0 / 1024.0;
    printf("so_far_bytes: %lld\n", so_far_bytes);
    printf("bufsize: %d kB RunTime: %.3f sec ThroughPut: %.3f MB/s\n", bufsize / 1024, run_time, tp);

    exit(0);
    
    return;
}

int main(int argc, char *argv[])
{
    int c;
    int n;
    unsigned char *buf;
    char *remote;
    int port;
    int debug = 0;
    int run_sec = 10;

    while ( (c = getopt(argc, argv, "dt:")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 't':
                run_sec = get_num(optarg);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 4) {
        usage();
        exit(1);
    }

    remote  = argv[0];
    port    = get_num(argv[1]);
    bufsize = get_num(argv[2]);

    bufsize = 1024*bufsize; /* kB */
    buf = (unsigned char *)malloc(bufsize);
    if (buf == NULL) {
        err(1, "malloc for buf");
    }
    memset(buf, 0, bufsize);
    
    my_signal(SIGALRM, print_result);
    set_timer(run_sec, 0, 0, 0);

    int sockfd = tcp_socket();
    if (connect_tcp(sockfd, remote, port) < 0) {
        errx(1, "connect_tcp");
    }
    
    gettimeofday(&begin, NULL);
    for ( ; ; ) {
        n = write(sockfd, buf, bufsize);
        so_far_bytes += n;
        if (n < 0) {
            err(1, "write");
        }
    }

    return 0;
}

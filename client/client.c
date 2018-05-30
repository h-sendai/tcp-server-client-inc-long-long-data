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
#include "bz_usleep.h"
#include "logUtil.h"
#include "set_cpu.h"

struct timeval begin, end;
unsigned long long so_far_bytes = 0;
int n_loop;
int bufsize;
int sockfd;

int usage()
{
    fprintf(stderr, "Usage: client [-b bufsize (1460)] [-c cpu_num ][-p port (1234)] [-s sleep_usec (0)] [-r so_rcvbuf] ip_address\n");
    fprintf(stderr, "use k, m for bufsize in kilo, mega\n");
    return 0;
}

void print_result(int signo)
{
    struct timeval diff;
    double run_time;
    double tp;

    int rcvbuf = get_so_rcvbuf(sockfd);
    fprintfwt(stderr, "client: SO_RCVBUF: %d (final)\n", rcvbuf);

    gettimeofday(&end, NULL);
    timersub(&end, &begin, &diff);
    run_time = diff.tv_sec + 0.000001*diff.tv_usec;
    tp = (double)so_far_bytes / run_time / 1024.0 / 1024.0;
    fprintfwt(stdout, "bufsize: %.3f kB RunTime: %.3f sec ThroughPut: %.3f MB/s\n", bufsize / 1024.0, run_time, tp);

    exit(0);
    
    return;
}

int main(int argc, char *argv[])
{
    int c;
    int n;
    unsigned char *buf;
    char *remote;
    int port = 1234;
    int debug = 0;
    int run_sec = 10;
    int sleep_usec = 0;
    int set_so_rcvbuf_size = 0;
    bufsize = 1460;
    int cpu_num = -1;

    while ( (c = getopt(argc, argv, "b:c:dhp:r:s:t:")) != -1) {
        switch (c) {
            case 'b':
                bufsize = get_num(optarg);
                break;
            case 'c':
                cpu_num = get_num(optarg);
                break;
            case 'd':
                debug = 1;
                break;
            case 'h':
                usage();
                exit(0);
                break;
            case 'p':
                port = strtol(optarg, NULL, 0);
                break;
            case 'r':
                set_so_rcvbuf_size = get_num(optarg);
                break;
            case 's':
                sleep_usec = get_num(optarg);
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

    if (argc != 1) {
        usage();
        exit(1);
    }

    remote  = argv[0];
    if (cpu_num != -1) {
        if (set_cpu(cpu_num) < 0) {
            errx(1, "set_cpu fail: cpu_num %d", cpu_num);
        }
    }

    buf = (unsigned char *)malloc(bufsize);
    if (buf == NULL) {
        err(1, "malloc for buf");
    }
    memset(buf, 0, bufsize);
    
    my_signal(SIGALRM, print_result);
    set_timer(run_sec, 0, 0, 0);

    sockfd = tcp_socket();
    if (set_so_rcvbuf_size > 0) {
        set_so_rcvbuf(sockfd, set_so_rcvbuf_size);
    }
    int rcvbuf = get_so_rcvbuf(sockfd);
    fprintfwt(stderr, "client: SO_RCVBUF: %d (init)\n", rcvbuf);

    if (connect_tcp(sockfd, remote, port) < 0) {
        errx(1, "connect_tcp");
    }
    
    gettimeofday(&begin, NULL);
    for ( ; ; ) {
        n = readn(sockfd, buf, bufsize);
        so_far_bytes += n;
        if (n < 0) {
            err(1, "read");
        }
        if (sleep_usec > 0) {
            bz_usleep(sleep_usec);
        }
        if (debug) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            fprintf(stderr, "%ld.%06ld read %d bytes\n", tv.tv_sec, tv.tv_usec, n);
        }
    }

    return 0;
}

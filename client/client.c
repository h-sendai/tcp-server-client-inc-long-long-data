#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <endian.h>

#include "get_num.h"
#include "host_info.h"
#include "my_signal.h"
#include "my_socket.h"
#include "readn.h"
#include "set_timer.h"
#include "bz_usleep.h"
#include "logUtil.h"
#include "set_cpu.h"

struct timeval begin, end, prev, now;
unsigned long long period_bytes = 0;
unsigned long long total_bytes  = 0;
int n_loop;
int bufsize;
int sockfd;
int debug = 0;
char *progname = NULL;

int usage()
{
    char msg[] = "Usage: client [options] ip_address\n"
                 "Connect to server and read data.  Display through put before exit.\n"
                 "\n"
                 "options:\n"
                 "-b BUFSIZE      read() buffer size (default: 16k). use k, m for kilo, mega\n"
                 "-c CPU_NUM      running cpu num (default: none)\n"
                 "-i BYTES        print interval transfer rate after (more than) BYTES read" 
                 "                if -t 0 (forever mode) (default 1GB)\n"
                 "-p PORT         port number (default: 1234)\n"
                 "-r SO_RCVBUF    Socket Recv Bufsize (default: os default)\n"
                 "-s SLEEP_USEC   sleep between each read() (default: don't sleep)\n"
                 "-t SECONDS      running period (default: 10 seconds.  0: forever)\n"
                 "-v              verify incremental data\n"
                 "-d              debug\n"
                 "-h              display this help\n";

    fprintf(stderr, "%s", msg);
    return 0;
}

void print_result(int signo)
{
    struct timeval diff;
    double run_time;
    double tp;

    int rcvbuf = get_so_rcvbuf(sockfd);
    fprintfwt(stderr, "%s: SO_RCVBUF: %d (final)\n", progname, rcvbuf);

    gettimeofday(&end, NULL);
    timersub(&end, &begin, &diff);
    run_time = diff.tv_sec + 0.000001*diff.tv_usec;
    tp = (double)total_bytes / run_time / 1024.0 / 1024.0;
    fprintfwt(stdout, "bufsize: %.3f kB RunTime: %.3f sec ThroughPut: %.3f MB/s\n", bufsize / 1024.0, run_time, tp);

    exit(0);
    
    return;
}

int print_period(struct timeval now, struct timeval prev)
{
    struct timeval diff;
    double run_time;
    double tp;

    timersub(&now, &prev, &diff);
    run_time = diff.tv_sec + 0.000001*diff.tv_usec;
    tp = (double)period_bytes / run_time / 1024.0 / 1024.0;
    
    fprintfwt(stdout, "%lld bytes read. %.3f MB/s\n", period_bytes, tp);

    return 0;
}

int verify_buf_inc_int(unsigned char *buf, int buflen)
{
    /*
     * if last read() remainder byte len = 1 and data = 0x12; then
     * remainder_buf[0] = 0x12;
     * remainder_len    = 1
     *
     * Next verify_buf_inc_int() has to pad remainder_buf[1,2,3] and decode it
     */

    // 0xff: enable finding bugs in early stage
    static unsigned char remainder_buf[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    static unsigned long remainder_len    = 0;
    static unsigned long x = 0;

    if (debug) {
        fprintf(stderr, "verify start. remainder_len: %lu\n", remainder_len);
    }

    /* 
     * remainder_len != 0: 
     * 前回の検証であまりバイトがあった場合の処理。
     * bufにデータが十分あり、intを作れる場合と、bufにデータがあまりなく
     * intを作れない場合がある。
     * intを作れる場合は作って、bufポインタを移動、次のforループでの検証に
     * すすむ。
     * intが作れなかった場合remainder_bufに入れて終了。
     */
    if (remainder_len != 0) {
        if (buflen >= (sizeof(long) - remainder_len)) { /* have enough data for padding */
            for (size_t i = 0; i < sizeof(long) - remainder_len; ++i) {
                remainder_buf[remainder_len + i] = buf[i];
                if (debug) {
                    fprintf(stderr, "padding at %ld\n", remainder_len + i);
                }
            }
            unsigned long *int_p = (unsigned long *)remainder_buf;
            if (x != be64toh(*int_p)) { // verificaiton failure
                fprintfwt(stderr, "%s: does not match: expected: %ld , got: %ld\n", progname, x, be64toh(*int_p));
                return -1;
            }
            else { // verification success
                if (debug) {
                    fprintf(stderr, "verified data: %lu\n", x);
                }
                x ++;
                buf += sizeof(long) - remainder_len;
                buflen -= (sizeof(long) - remainder_len);
            }
        }
        else { // padding only 
            for (size_t i = 0; i < buflen; ++i) {
                remainder_buf[remainder_len + i] = buf[i];
                if (debug) {
                    fprintf(stderr, "padding at %ld\n", remainder_len + i);
                }
                remainder_len ++;
                return 0;
            }
        }
    }

    unsigned long *int_p = (unsigned long *)buf;

    /*
     * このforループでintサイズ分どんどん検証する。
     * あまりがあるかどうか調べるのはこのforループがぬけたあと。
     */
    for (size_t i = 0; i < buflen/sizeof(long); ++i) {
        // if ( x != ntohl(*int_p) ) {
        if ( x != be64toh(*int_p) ) {
            fprintfwt(stderr, "%s: does not match: expected: %lu , got: %lu\n", progname, x, be64toh(*int_p));
            return -1;
        }
        else {
            if (debug) {
                fprintf(stderr, "verified data: %lu\n", x);
            }
        }
        x ++;
        int_p ++;
    }

    /*
     * あまりバイトがあるかどうか調べて、あったらremainder_len, remainder_buf[]に
     * 格納。次回のverify_buf_inc_int()呼び出しで使う。
     */
    remainder_len = (buflen % sizeof(long));
    if (debug) {
        fprintf(stderr, "next remainder_len: %lu\n", remainder_len);
    }
    if (buflen % sizeof(long) != 0) {
        for (size_t i = 0; i < remainder_len; ++i) {
            //if (debug) {
                //fprintf(stderr, "i: %ld\n", i);
            //}
            unsigned int j = (buflen/sizeof(long))*sizeof(long) + i;
            remainder_buf[i] = buf[j];
            if (debug) {
                fprintf(stderr, "padding at %ld\n", i);
            }
            //if (debug) {
            //    fprintf(stderr, "buf index: %u\n", j);
            //}
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int c;
    int n;
    unsigned char *buf;
    char *remote;
    int port = 1234;
    int run_sec = 10;
    int sleep_usec = 0;
    int set_so_rcvbuf_size = 0;
    bufsize = 16*1024;
    int cpu_num = -1;
    int do_verify = 0;
    unsigned long long interval_bytes = 1*1024*1024*1024; /* 1GB */

    progname = strrchr(argv[0], '/') + 1;

    while ( (c = getopt(argc, argv, "b:c:dhi:p:r:s:t:v")) != -1) {
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
            case 'i':
                interval_bytes = get_num(optarg);
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
            case 'v':
                do_verify = 1;
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
    
    if (run_sec > 0) {
        my_signal(SIGALRM, print_result);
        set_timer(run_sec, 0, 0, 0);
    }
    my_signal(SIGINT, print_result);

    sockfd = tcp_socket();
    if (set_so_rcvbuf_size > 0) {
        set_so_rcvbuf(sockfd, set_so_rcvbuf_size);
    }
    int rcvbuf = get_so_rcvbuf(sockfd);
    fprintfwt(stderr, "%s: SO_RCVBUF: %d (init)\n", progname, rcvbuf);

    if (connect_tcp(sockfd, remote, port) < 0) {
        errx(1, "connect_tcp");
    }
    
    gettimeofday(&begin, NULL);
    prev = begin;
    unsigned long read_count = 0;
    for ( ; ; ) {
        n = readn(sockfd, buf, bufsize);
        if (debug) {
            fprintf(stderr, "---> read_count: %ld read bytes: %d\n", read_count, n);
        }
        read_count += 1;
        total_bytes += n;
        period_bytes += n;
        if (n < 0) {
            err(1, "read");
        }
        if (sleep_usec > 0) {
            bz_usleep(sleep_usec);
        }
        //if (debug) {
        //    struct timeval tv;
        //    gettimeofday(&tv, NULL);
        //    fprintf(stderr, "%ld.%06ld read %d bytes\n", tv.tv_sec, tv.tv_usec, n);
        //}
        if (do_verify) {
            if (verify_buf_inc_int(buf, n) < 0) { // use n: may use read() instead of readn()
                exit(0);
            }
        }
        if (run_sec == 0) {
            if (period_bytes > interval_bytes) {
                gettimeofday(&now, NULL);
                print_period(now, prev);
                period_bytes = 0;
                prev = now;
            }
        }
    }

    return 0;
}

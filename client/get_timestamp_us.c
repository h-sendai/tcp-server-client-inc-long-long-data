#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int get_timestamp_us(char *buf, int len)
{
    struct timeval tv;
    struct tm      tm;
    int tmp_len;
    
    char *tmp = malloc(128);

    if (tmp == NULL) {
        warn("get_timestamp_ms: malloc");
        return -1;
    }
    memset(tmp, 0, 128);
    if (gettimeofday(&tv, NULL) < 0) {
        warn("get_timestamp_ms: gettimeofday()");
        return -1;
    }
    localtime_r(&tv.tv_sec, &tm);
    strftime(tmp, 128, "%F %T", &tm);
    tmp_len = strlen(tmp);
    // 2013-10-05 07:39:11.123456\0
    if (len < tmp_len + 8) {
        warnx("not enough buffer: %d bytes specified", len);
        return -1;
    }
    snprintf(buf, len, "%s.%06ld", tmp, tv.tv_usec);
    free(tmp);

    return 0;
}

/*
 * int main(int argc, char *argv[])
 * {
 *   char timestamp[27];
 *
 *    get_timestamp_us(timestamp, sizeof(timestamp));
 *   printf("%s\n", timestamp);
 *
 *   return 0;
 * }
*/

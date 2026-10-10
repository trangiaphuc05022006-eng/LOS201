#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "asgn1_ioctl.h"

/* usage: asgn1_ctl reset | stats | mode N | version | bogus */

int main(int argc, char *argv[])
{
    int fd, ret = 0;
    struct asgn1_stats st;
    char version[ASGN1_VERSION_LEN];          /* must match the size inside the command */
    int mode;

    if (argc < 2) {
        fprintf(stderr, "usage: %s reset|stats|mode N|version|bogus\n", argv[0]);
        return 1;
    }

    fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open failed: %s\n", strerror(errno));
        return 1;
    }

    if (strcmp(argv[1], "reset") == 0) {
        ret = ioctl(fd, ASGN1_RESET_BUFFER, 0);                 /* no data */
    } else if (strcmp(argv[1], "stats") == 0) {
        ret = ioctl(fd, ASGN1_GET_STATS, &st);           /* kernel fills st */
        if (ret == 0) {
            printf("open_count      : %d\n", st.open_count);
            printf("read_count      : %d\n", st.read_count);
            printf("write_count     : %d\n", st.write_count);
            printf("buffer_len      : %d\n", st.buffer_len);
            printf("last_write_size : %d\n", st.last_write_size);
        }
    } else if (strcmp(argv[1], "mode") == 0) {
        if (argc < 3) { fprintf(stderr, "mode needs 0 or 1\n"); close(fd); return 1; }
        mode = atoi(argv[2]);
        ret = ioctl(fd, ASGN1_SET_MODE, &mode);           /* we send mode in */
    } else if (strcmp(argv[1], "version") == 0) {
        memset(version, 0, sizeof(version));
        ret = ioctl(fd, ASGN1_GET_VERSION, version);           /* kernel fills version */
        if (ret == 0)
            printf("version: %s\n", version);
    } else if (strcmp(argv[1], "bogus") == 0) {
        ret = ioctl(fd, _IO(ASGN1_MAGIC, 99), 0);                /* command number nobody handles */
    }

    if (ret < 0) {
        fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "asgn1_ioctl.h"

/*
 * main - command-line control utility for /dev/asgn1
 *
 * Purpose:    Opens the device and sends one ioctl command to the driver,
 *             chosen by the first argument.
 * Usage:      asgn1_ctl reset | stats | mode N | version | bogus
 * Parameters: argc - number of command-line arguments
 *             argv - argv[1] is the subcommand; argv[2] is the mode number
 *                    when argv[1] is "mode"
 * Return:     0 on success, 1 on bad usage, open failure or ioctl failure
 */
int main(int argc, char *argv[])
{
    int fd, ret = 0;
    struct asgn1_stats st;
    char version[ASGN1_VERSION_LEN];          /* must match the size inside the command */
    int mode;
    char *end;
    long val;

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
        ret = ioctl(fd, ASGN1_GET_STATS, &st);                  /* kernel fills st */
        if (ret == 0) {
            printf("open_count      : %d\n", st.open_count);
            printf("read_count      : %d\n", st.read_count);
            printf("write_count     : %d\n", st.write_count);
            printf("buffer_len      : %d\n", st.buffer_len);
            printf("last_write_size : %d\n", st.last_write_size);
        }
    } else if (strcmp(argv[1], "mode") == 0) {
        if (argc < 3) {
            fprintf(stderr, "mode needs 0 or 1\n");
            close(fd);
            return 1;
        }
        errno = 0;
        val = strtol(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' ||
            errno == ERANGE || val < INT_MIN || val > INT_MAX) {
            fprintf(stderr, "mode needs a number, got '%s'\n", argv[2]);
            close(fd);
            return 1;
        }
        mode = (int)val;                      /* any number is passed on; the driver rejects bad ones */
        ret = ioctl(fd, ASGN1_SET_MODE, &mode);                 /* we send mode in */
    } else if (strcmp(argv[1], "version") == 0) {
        memset(version, 0, sizeof(version));
        ret = ioctl(fd, ASGN1_GET_VERSION, version);            /* kernel fills version */
        if (ret == 0)
            printf("version: %s\n", version);
    } else if (strcmp(argv[1], "bogus") == 0) {
        ret = ioctl(fd, _IO(ASGN1_MAGIC, 99), 0);               /* command number nobody handles */
    } else {
        fprintf(stderr, "usage: %s reset|stats|mode N|version|bogus\n", argv[0]);
        close(fd);
        return 1;                             /* unknown subcommand must not look like success */
    }

    if (ret < 0) {
        fprintf(stderr, "ioctl failed (%s): %s\n", argv[1], strerror(errno));
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}
/*
 * File: ioctl_app.c
 * Author: Tran Gia Phuc - SE204417
 * Description: Userspace CLI helper to invoke the 4 ioctl commands
 *              on /dev/asgn1 from test_asgn1.sh script.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "asgn1_ioctl.h"

#define DEV_PATH "/dev/asgn1"

int main(int argc, char *argv[])
{
    int fd, ret, mode;
    struct asgn1_stats stats;
    char version[ASGN1_VERSION_MAX_LEN];

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <reset|stats|mode <0|1>|version|invalid>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(DEV_PATH, O_RDWR);
    if (fd < 0) {
        perror("open /dev/asgn1");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "reset") == 0) {
        ret = ioctl(fd, ASGN1_RESET_BUFFER);
        if (ret < 0) {
            perror("ioctl(ASGN1_RESET_BUFFER)");
            close(fd);
            return EXIT_FAILURE;
        }
        printf("[USERSPACE] ASGN1_RESET_BUFFER executed successfully.\n");

    } else if (strcmp(argv[1], "stats") == 0) {
        memset(&stats, 0, sizeof(stats));
        ret = ioctl(fd, ASGN1_GET_STATS, &stats);
        if (ret < 0) {
            perror("ioctl(ASGN1_GET_STATS)");
            close(fd);
            return EXIT_FAILURE;
        }
        printf("[USERSPACE] ASGN1_GET_STATS result:\n");
        printf("  -> open_count      : %d\n", stats.open_count);
        printf("  -> write_count     : %d\n", stats.write_count);
        printf("  -> read_count      : %d\n", stats.read_count);
        printf("  -> buffer_len      : %d bytes\n", stats.buffer_len);
        printf("  -> last_write_size : %d bytes\n", stats.last_write_size);

    } else if (strcmp(argv[1], "mode") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: missing mode value (0 or 1)\n");
            close(fd);
            return EXIT_FAILURE;
        }
        mode = atoi(argv[2]);
        ret = ioctl(fd, ASGN1_SET_MODE, &mode);
        if (ret < 0) {
            perror("ioctl(ASGN1_SET_MODE)");
            close(fd);
            return EXIT_FAILURE;
        }
        printf("[USERSPACE] ASGN1_SET_MODE set to %d (%s).\n",
               mode, (mode == ASGN1_MODE_KERNEL) ? "Kernel mode" : "Echo mode");

    } else if (strcmp(argv[1], "version") == 0) {
        memset(version, 0, sizeof(version));
        ret = ioctl(fd, ASGN1_GET_VERSION, version);
        if (ret < 0) {
            perror("ioctl(ASGN1_GET_VERSION)");
            close(fd);
            return EXIT_FAILURE;
        }
        printf("[USERSPACE] ASGN1_GET_VERSION: \"%s\"\n", version);

    } else if (strcmp(argv[1], "invalid") == 0) {
        /* Gửi một mã lệnh ioctl giả mạo để kiểm tra driver trả về -ENOTTY */
        ret = ioctl(fd, _IO('Z', 99));
        if (ret < 0) {
            printf("[USERSPACE] Invalid ioctl correctly rejected: %s (errno=%d)\n",
                   strerror(errno), errno);
        }
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "sms_sensor.h"

int main(void)
{
    int vals[] = { 500, 5, 70000 };
    int fd = open("/dev/sms_sensor", O_RDONLY);

    /* TODO 1: if fd < 0 -> perror("open") and return 1 */
    if (fd <0) { 
        perror("open");
        return 1;
    }

    /* TODO 2: for each value: ioctl(fd, SMS_SET_INTERVAL, &vals[i]);
     *         print the value, the return value, and strerror(errno)
     *         when the call failed */
    for (int i = 0; i < (sizeof(vals) / sizeof(vals[0])); i++) {
        int ret = ioctl(fd, SMS_SET_INTERVAL, &vals[i]);
        printf("vals[%d]: %d, return: %d\n", i, vals[i], ret);
        if (ret < 0) {
            printf("error: %s\n", strerror(errno));
        }
    }

    /* TODO 3: close fd */
    close(fd); 
    return 0;
}
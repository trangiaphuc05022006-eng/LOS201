#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    const size_t size = 10 * 1024 * 1024;
    char *buffer = malloc(size);

    if (!buffer) {
        perror("malloc");
        return 1;
    }

    for (size_t i = 0; i < size; i += 4096)
        buffer[i] = 1;

    printf("PID = %ld\n", (long)getpid());
    fflush(stdout);

    sleep(30);
    free(buffer);
    return 0;
}

#include "alert_thread.h"
#include <stdio.h>
#include <string.h>

extern volatile int keep_running;

void *alert_thread_func(void *arg) {
    AlertThreadArgs *args = (AlertThreadArgs *)arg;
    char buffer[256];
    unsigned int prio;

    while (keep_running) {
        // mq_receive sẽ block luồng này lại cho đến khi có tin nhắn trong alarm_queue
        ssize_t bytes = mq_receive(args->alarm_mq, buffer, sizeof(buffer), &prio);
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            
            // In cảnh báo ra stderr theo yêu cầu Task 1
            fprintf(stderr, "%s\n", buffer);
            
            // Chuyển tiếp tin nhắn sang log_queue để Logger Thread ghi vào file
            mq_send(args->log_mq, buffer, strlen(buffer), 0);
        }
    }
    
    return NULL;
}

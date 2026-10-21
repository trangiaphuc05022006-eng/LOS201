#include "processor_thread.h"
#include <stdio.h>
#include <math.h>
extern volatile int keep_running;
void *processor_thread_func(void *arg) {
    ProcessorThreadArgs *args = (ProcessorThreadArgs *)arg;
    SensorData data;
    float last_temp = 0.0;
    int first_sample = 1;
    while (keep_running) {
        if (cb_pop(args->cb, &data) == 0) {
            if (data.temperature > args->temp_threshold) {
                printf("[Processor] ALARM: Nhiệt độ quá cao! %.1f°C (Ngưỡng: %.1f°C)\n", 
                       data.temperature, args->temp_threshold);
            }
            if (data.humidity > args->hum_threshold) {
                printf("[Processor] ALARM: Độ ẩm quá cao! %.1f%% (Ngưỡng: %.1f%%)\n", 
                       data.humidity, args->hum_threshold);
            }
            if (!first_sample && fabs(data.temperature - last_temp) > 5.0) {
                printf("[Processor] ALARM: Nhiệt độ thay đổi đột biến! Mới: %.1f°C, Cũ: %.1f°C\n", 
                       data.temperature, last_temp);
            }
            last_temp = data.temperature;
            first_sample = 0;
        }
    }
    return NULL;
}

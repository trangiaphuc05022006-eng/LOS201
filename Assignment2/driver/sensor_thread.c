#include "sensor_thread.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// Biến cờ toàn cục báo hiệu dừng chương trình (sẽ viết ở hàm main sau)
extern volatile int keep_running;

void *sensor_thread_func(void *arg) {
    SensorThreadArgs *args = (SensorThreadArgs *)arg;
    
    // Mở thiết bị driver đã cấp quyền lúc nãy
    int fd = open("/dev/sms_sensor", O_RDONLY);
    if (fd < 0) {
        perror("Loi: Khong the mo /dev/sms_sensor");
        return NULL;
    }

    char buf[128];
    SensorData data;

    // Vòng lặp chạy liên tục cho đến khi có lệnh tắt (Ctrl+C)
    while (keep_running) {
        lseek(fd, 0, SEEK_SET); // Đưa con trỏ file về đầu trước mỗi lần đọc
        int bytes = read(fd, buf, sizeof(buf) - 1);
        
        if (bytes > 0) {
            buf[bytes] = '\0'; // Kết thúc chuỗi
            
            // Parse dữ liệu từ chuỗi CSV
            if (sscanf(buf, "%lu,%f,%f", &data.timestamp, &data.temperature, &data.humidity) == 3) {
                if (args->verbose) {
                    printf("[Sensor Thread] Đã đọc: T=%.1f°C, H=%.1f%%\n", data.temperature, data.humidity);
                }
                
                // Đẩy vào buffer, nếu đầy thì báo lỗi theo yêu cầu Task 1
                if (cb_push(args->cb, data) != 0) {
                    fprintf(stderr, "[Cảnh báo] Buffer đầy, bỏ qua sample hiện tại!\n");
                }
            }
        }
        // Ngủ đông chờ tới chu kỳ tiếp theo
        usleep(args->interval_ms * 1000);
    }

    close(fd);
    return NULL;
}

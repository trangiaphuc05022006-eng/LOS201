#ifndef APP_SMS_SENSOR_H
#define APP_SMS_SENSOR_H
#include <stdint.h>
typedef struct {
    uint64_t timestamp;
    float temperature;
    float humidity;
} SensorData;
#endif

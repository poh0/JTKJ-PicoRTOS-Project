#pragma once

#include <inttypes.h>

// tänne IMUa koskevat määritelmät, funktiot ym,
// jotka halutaan, että näkyy ulospäin.

typedef struct {
    float x;
    float y;
    float z;
} gyro_data_t;

typedef struct {
    float x;
    float y;
    float z;
} acc_data_t;

typedef struct {
    gyro_data_t gyro;
    acc_data_t acc;
    uint32_t timestamp;
} imu_data_t;

void init_imu(void);
void start_imu(void);
void stop_imu(void);
void imu_read_raw(imu_data_t *data_out);


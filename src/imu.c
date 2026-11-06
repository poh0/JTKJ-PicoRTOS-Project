#include "tkjhat/sdk.h"

#include "imu.h"

// tänne imun funktioiden toteutukset

void init_imu(void) {
    if (init_ICM42670() != 0) {
        // error
    }

}

void start_imu(void) {
    // Käynnistetään anturi (Low-Noise -tila, oletus ODR ja asteikot)
    ICM42670_start_with_default_values();
    // Lyhyt asettumisaika, jotta gyro ja kiihtyvyysanturi ehtivät herätä
    sleep_ms(10); 
}

void stop_imu(void) {
    // Asetetaan gyroskooppi ja kiihtyvyysanturi pois päältä (PWR_MGMT0 = 0x00)
    // Tällä anturi saadaan nukkumaan tiloissa, joissa ei mitata
    uint8_t buf[2] = { ICM42670_PWR_MGMT0_REG, 0x00 };
    i2c_write_blocking(i2c_default, ICM42670_I2C_ADDRESS, buf, 2, false);
}

void imu_read_raw(imu_data_t *data_out) {
    float temp;
    int result = ICM42670_read_sensor_data(
        &data_out->acc.x,
        &data_out->acc.y,
        &data_out->acc.z,
        &data_out->gyro.x,
        &data_out->gyro.y,
        &data_out->gyro.z,
        &temp
    );

    // handle result    

    return;
}
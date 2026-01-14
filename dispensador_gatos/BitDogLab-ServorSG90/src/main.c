#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "init.h"
#include "vl53l0x_c.h"

#define I2C_PORT i2c0
#define I2C_SDA  0
#define I2C_SCL  1

#define DISTANCIA_TRIGGER_MM 200

void i2c0_init_sensor(void) {
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    sleep_ms(100);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    printf("Inicializando sistema...\n");

    // Servo
    initializeSystem();
    servo_set_angle(0);

    // I2C + Sensor
    i2c0_init_sensor();

    vl53l0x_init();
    vl53l0x_start(100);

    printf("Sensor VL53L0X pronto!\n");

    while (1) {
        uint16_t dist = vl53l0x_read_mm();

        if (!vl53l0x_timeout()) {
            printf("Distancia: %u mm\n", dist);

            if (dist < DISTANCIA_TRIGGER_MM) {
                servo_set_angle(90);
                sleep_ms(3000);
                servo_set_angle(0);
            }
        } else {
            printf("Timeout do sensor!\n");
        }

        sleep_ms(200);
    }
}

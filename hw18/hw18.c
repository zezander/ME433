#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <math.h>
#include "hardware/pwm.h"

// for AS5600
#define I2C_PORT i2c0
#define SDA_PIN 4
#define SCL_PIN 5
#define AS5600_ADDR 0x36
#define AS5600_ANGLE_H 0x0E

//for motor control (PWM)
#define PWM_PIN1 14
#define PWM_PIN2 15
#define MAX_PWM 65535

// for HX711
#define HX711_DT 16
#define HX711_SCK 17

//for centering the arm and setting the control range
#define CENTER_DEG 173.0f
#define RANGE_DEG 45.0f

//for motor control (set PWM duty cycle)
void set_motor(float u) {
    // u is from -1.0 to 1.0
    if (u > 1.0) u = 1.0;
    if (u < -1.0) u = -1.0;

    uint16_t pwm = (uint16_t)(fabs(u) * MAX_PWM);
    if (u > 0) {
        pwm_set_gpio_level(PWM_PIN1, pwm);
        pwm_set_gpio_level(PWM_PIN2, 0);
    } else if (u < 0) {
        pwm_set_gpio_level(PWM_PIN1, 0);
        pwm_set_gpio_level(PWM_PIN2, pwm);
    } else {
        pwm_set_gpio_level(PWM_PIN1, 0);
        pwm_set_gpio_level(PWM_PIN2, 0);
    }
}

//for AS5600 (read angle values) 
uint16_t read_as5600_angle() {
    uint8_t reg = AS5600_ANGLE_H;
    uint8_t data[2];

    i2c_write_blocking(I2C_PORT, AS5600_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, AS5600_ADDR, data, 2, false);

    uint16_t angle = ((data[0] & 0x0F) << 8) | data[1];
    return angle; // 0 to 4095
}

int32_t read_hx711() {
    int32_t raw = 0;

    while (gpio_get(HX711_DT) == 1) {
        tight_loop_contents();
    }

    for (int i = 0; i < 24; i++) {
        gpio_put(HX711_SCK, 1);
        sleep_us(1);

        raw = raw << 1;

        gpio_put(HX711_SCK, 0);
        sleep_us(1);

        if (gpio_get(HX711_DT)) {
            raw++;
        }
    }

    gpio_put(HX711_SCK, 1);
    sleep_us(1);
    gpio_put(HX711_SCK, 0);
    sleep_us(1);

    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }

    return raw;
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    // I2C setup
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // HX711 setup
    gpio_init(HX711_DT);
    gpio_set_dir(HX711_DT, GPIO_IN);

    gpio_init(HX711_SCK);
    gpio_set_dir(HX711_SCK, GPIO_OUT);
    gpio_put(HX711_SCK, 0);

    //for pwm setup
    gpio_set_function(PWM_PIN1, GPIO_FUNC_PWM);
    gpio_set_function(PWM_PIN2, GPIO_FUNC_PWM);

    uint slice_num1 = pwm_gpio_to_slice_num(PWM_PIN1);
    uint slice_num2 = pwm_gpio_to_slice_num(PWM_PIN2);
    
    pwm_set_wrap(slice_num1, MAX_PWM);
    pwm_set_wrap(slice_num2, MAX_PWM);
    
    pwm_set_enabled(slice_num1, true);
    pwm_set_enabled(slice_num2, true);

    printf("angle_deg,load_raw\n");

    while (true) {
        uint16_t angle_raw = read_as5600_angle();
        float angle_deg = angle_raw * 360.0f / 4096.0f;

        int32_t load_raw = read_hx711();

        float x = (angle_deg - CENTER_DEG) / RANGE_DEG;

        if (x > 1.0f) x = 1.0f;
        if (x < -1.0f) x = -1.0f;

        float K = 0.3f;
        float u = K * x;

        set_motor(u);

        printf("%.2f,%ld,%.3f,%.3f\n", angle_deg, load_raw, x, u);
        sleep_ms(10);
        
    }
}
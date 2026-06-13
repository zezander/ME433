#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include "pico/time.h"
#include "ssd1306.h"
#include "font.h"

// for imu
#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1
#define MPU_ADDR 0x68


//registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C

//sensor data registers
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define WHO_AM_I     0x75

void init_imu();
void read_imu(float *accel, float *gyro, float *temp);
void drawChar(unsigned char x, unsigned char y, char c);
void drawString(unsigned char x, unsigned char y, char *message);
void drawLine(int x0, int y0, int x1, int y1, int color);


int main()
{
    stdio_init_all();
    sleep_ms(2000);

    //setting up i2c for imu and oled
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ssd1306_setup();

    //check who am i
    uint8_t reg = WHO_AM_I;
    uint8_t who_am_i = 0;
    i2c_write_blocking(I2C_PORT, MPU_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU_ADDR, &who_am_i, 1, false);

    //printf("WHO_AM_I: 0x%X\n", who_am_i);

    if (who_am_i != 0x68 && who_am_i != 0x98) {
        while(1) {
            printf("MPU not found! WHO_AM_I = 0x%02X\n", who_am_i);
            sleep_ms(1000);
        }
    }

    init_imu();

    float accel[3];
    float gyro[3];
    float temp;

    char fpsmsg[50];
    unsigned int t_old = to_us_since_boot(get_absolute_time());


    while (1) {

        read_imu(accel, gyro, &temp);

        //fps calculation
        unsigned int t_new = to_us_since_boot(get_absolute_time());
        float fps = 1000000.0 / (t_new - t_old);
        t_old = t_new;
        sprintf(fpsmsg, "FPS: %.1f", fps);

        //oled center 
        int x0=64;
        int y0=16;

        //scaling accelfor oled display screen 
        int x1 = x0 + (int)(accel[0]*20.0f);
        int y1 = y0 + (int)(accel[1]*20.0f);

        //endpoints for oled display screen 
        if (x1 < 0) x1 = 0;
        if (x1 > 127) x1 = 127;
        if (y1 < 0) y1 = 0;
        if (y1 > 31) y1 = 31;

        ssd1306_clear();

        //draw center point
        ssd1306_drawPixel(x0, y0, 1);
        
        //draw line for tilt 
        drawLine(x0, y0, x1, y1, 1);
        
        //draw fps
        drawString(0, 0, fpsmsg);

        ssd1306_update();


        printf("ax=%.3f ay=%.3f az=%.3f   gx=%.2f gy=%.2f gz=%.2f   temp=%.2f\n",
               accel[0], accel[1], accel[2],
               gyro[0], gyro[1], gyro[2],
               temp);
        sleep_ms(10); //100 Hz
    }
}


// function to turn on imu 
void init_imu() {
    uint8_t buf[2];

    // turn on IMU
    buf[0] = PWR_MGMT_1;
    buf[1] = 0x00;
    i2c_write_blocking(i2c0, 0x68, buf, 2, false);

    // Set accelerometer to +/-2g
    buf[0] = ACCEL_CONFIG;
    buf[1] = 0x00;
    i2c_write_blocking(i2c0, 0x68, buf, 2, false);

    // Set gyro to +/-2000 dps
    buf[0] = GYRO_CONFIG;
    buf[1] = 0x18;
    i2c_write_blocking(i2c0, 0x68, buf, 2, false);
}

// function to read imu data
void read_imu(float *accel, float *gyro, float *temp) {
    uint8_t reg = ACCEL_XOUT_H;
    uint8_t data[14];

    // Tell MPU where to start reading
    i2c_write_blocking(i2c0, 0x68, &reg, 1, true);

    // Read 14 bytes
    i2c_read_blocking(i2c0, 0x68, data, 14, false);

    // Combine bytes
    int16_t ax = (data[0] << 8) | data[1];
    int16_t ay = (data[2] << 8) | data[3];
    int16_t az = (data[4] << 8) | data[5];

    int16_t gx = (data[8] << 8) | data[9];
    int16_t gy = (data[10] << 8) | data[11];
    int16_t gz = (data[12] << 8) | data[13];

    int16_t temp_raw = (data[6] << 8) | data[7];

    // converting units
    accel[0] = ax * 0.000061;
    accel[1] = ay * 0.000061;
    accel[2] = az * 0.000061;

    gyro[0] = gx * 0.00763;
    gyro[1] = gy * 0.00763;
    gyro[2] = gz * 0.00763;

    *temp = temp_raw/ 340.0 + 36.53;
    
}

//----from HW4.c 

//function to draw letter at (x,y) using the ASCII lookup table (pt. 2)
void drawChar(unsigned char x, unsigned char y, char c) {
    int letter = c - 0x20;   // convert ASCII character to font index

    if (letter < 0 || letter > 95) {
        return;   // ignore unsupported characters
    }

    for (int col = 0; col < 5; col++) { // each character is 5 columns wide
        char column_data = ASCII[letter][col];  // get the column data for character

        for (int row = 0; row < 8; row++) { // each column is 8 bits tall
            int bit = (column_data >> row) & 0x01; //for pixel on/off
            ssd1306_drawPixel(x + col, y + row, bit);
        }
    }
}

//function to draw string at (x,y) using drawChar (pt. 3)
void drawString(unsigned char x, unsigned char y, char *message) {
    int i = 0;

    while (message[i] != '\0') {
        drawChar(x + i * 6, y, message[i]);  // move right each letter
        i++;
    }
}


//new function for line 
void drawLine(int x0, int y0, int x1, int y1, int color) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    int steps;
    if (abs(dx) > abs(dy)) {
        steps = abs(dx);
    } else {
        steps = abs(dy);
    }

    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;

    float x = x0;
    float y = y0;

    for (int i = 0; i <= steps; i++) {
        ssd1306_drawPixel((int)x, (int)y, color);
        x += xinc;
        y += yinc;
    }
}
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include "ssd1306.h"
#include "font.h"


#define LED_PIN 25
#define SDA_PIN 0
#define SCL_PIN 1


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
void i2c_scan() {
    printf("\nI2C scan starting...\n");

    for (int addr = 0; addr < 127; addr++) {
        int ret = i2c_write_blocking(i2c_default, addr, NULL, 0, false);

        if (ret >= 0) {
            printf("Found device at 0x%02X\n", addr);
        }
    }

    printf("I2C scan done.\n");
}

int main() {
    stdio_init_all();

    // setting pico led
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // setting I2C for oled
    i2c_init(i2c_default, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    printf("Starting scan...\n");
    i2c_scan();

    // OLED init
    ssd1306_setup();

    int state = 0;

    //for strin message (pt. 3)
    //int i = 15;
    //char message[50];

    // init ADC for reading potentiometer value (pt. 4)
    adc_init();
    adc_gpio_init(26);   // ADC0 = GPIO26
    adc_select_input(0);
    char msg[50];
    char fpsmsg[50]; 
    unsigned int t_old = to_us_since_boot(get_absolute_time());  

    while (1) {
        //blink pico led 
        state = !state;
        gpio_put(LED_PIN, state);

        //-----blink oled pixel (pt. 1)-----
        //ssd1306_clear();
        //ssd1306_drawPixel(10, 10, state);
        //ssd1306_update();

        //-----draw character (pt. 2)-----
        //ssd1306_clear();
        //drawChar(10, 10, 'A');
        //ssd1306_update();
        //sleep_ms(500);
        //gpio_put(25, 0);
        //ssd1306_clear();
        //ssd1306_update();

        //-----draw string (pt. 3)-----
        //sprintf(message, "Hello %d", i);  // create message with changing number
        //ssd1306_clear();
        //drawString(10, 10, message);  // draw the message on the OLED
        //ssd1306_update();
        //sleep_ms(500);
        //gpio_put(25, 0);

        //-----read ADC and display (pt. 4)-----   
        uint16_t adc_value = adc_read();  // read potentiometer value (0-4095)
        float voltage = adc_value * 3.3f / 4095.0f;  // convert to voltage

        //FPS calc
        unsigned int t_new = to_us_since_boot(get_absolute_time());
        float fps = 1000000.0f / (t_new - t_old);
        t_old = t_new;

        //Text to display
        sprintf(msg, "ADC: %.2f", voltage);
        sprintf(fpsmsg, "FPS: %.1f", fps);

        ssd1306_clear();
        drawString(0, 0, msg);  // draw ADC value
        drawString(0, 24, fpsmsg); // draw FPS
        ssd1306_update();
        sleep_ms(100);
    }
}

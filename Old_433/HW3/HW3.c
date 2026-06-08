#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define LED_PIN 15       // GPIO 15 (pin 20)
#define BUTTON_PIN 14    // GPIO 14 (pin 19)

volatile bool toggle_led = true;
volatile bool button_flag = false;

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        toggle_led = !toggle_led;
        button_flag = true;
    }
}

int main() {
    stdio_init_all();

    // wait for usb to connect
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    // print start to let user know
    printf("Start!\n");

    // init buttons and leds
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, toggle_led);  // Turn LED ON

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);  // Active-low button
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    //init adc
    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0

    while (1) {
        gpio_put(LED_PIN, toggle_led);
        if (button_flag) {
            int sample; 
            // prompt user to enter sample, read from screen, read back touser
            printf("Enter the number of desired samples between 1 and 100:\n");
            scanf("%d", &sample);
            printf("You entered: %d\n", sample);
            printf("Printing potentiometer voltages: \n");

            // for loop to get voltages at 100hz
            float samples[sample];
            for (int i = 0; i < sample; i++) {
                uint16_t result = adc_read();
                samples[i] = 0.0008 * result;
                printf("%f\n",samples[i]);
                sleep_ms(10);
            }
            printf("Press button again to repeat.\n");
            toggle_led = !toggle_led; // turn LED on to signal that button can be pressed
            button_flag = false; // Clear the flag
        }
        sleep_ms(50);
    }
}

static const char *gpio_irq_str[] = {
    "LEVEL_LOW",  // 0x1
    "LEVEL_HIGH", // 0x2
    "EDGE_FALL",  // 0x4
    "EDGE_RISE"   // 0x8
};
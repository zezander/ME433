// /**
//  * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
//  *
//  * SPDX-License-Identifier: BSD-3-Clause
//  */

// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "hardware/gpio.h"

// #define LED_PIN 15       // GPIO 15 (pin 20)
// #define BUTTON_PIN 14    // GPIO 14 (pin 19)
// #define LED_DELAY_MS 1000
// #define GPIO_WATCH_PIN 2

// // static char event_str[128];

// // void gpio_event_string(char *buf, uint32_t events);
// volatile int press_count = 0;
// volatile bool toggle_led = false;

// void gpio_callback(uint gpio, uint32_t events) {
//     if (gpio == BUTTON_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
//         press_count++;
//         toggle_led = !toggle_led;
//         printf("Button pressed %d times\n", press_count);
//     }
// }

// int main() {
//     stdio_init_all();  // Enable USB serial output

//     gpio_init(LED_PIN);
//     gpio_set_dir(LED_PIN, GPIO_OUT);
//     gpio_put(LED_PIN, false);  // Start with LED off

//     gpio_init(PICO_DEFAULT_LED_PIN);
//     gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
//     gpio_put(PICO_DEFAULT_LED_PIN, 1);

//     gpio_init(BUTTON_PIN);
//     gpio_set_dir(BUTTON_PIN, GPIO_IN);
//     gpio_pull_up(BUTTON_PIN);  // Active-low button3438259548

//     // Set up the button interrupt on falling edge (button press)
//     gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

//     // while (true) {
//     //     gpio_put(LED_PIN, toggle_led);
//     //     sleep_ms(10);  // Tiny sleep to reduce CPU usage
//     // }
//     while (true) {
//         gpio_put(LED_PIN, 1);
//     }
// }

// static const char *gpio_irq_str[] = {
//     "LEVEL_LOW",  // 0x1
//     "LEVEL_HIGH", // 0x2
//     "EDGE_FALL",  // 0x4
//     "EDGE_RISE"   // 0x8
// };

// // void gpio_event_string(char *buf, uint32_t events) {
// //     for (uint i = 0; i < 4; i++) {
// //         uint mask = (1 << i);
// //         if (events & mask) {
// //             // Copy this event string into the user string
// //             const char *event_str = gpio_irq_str[i];
// //             while (*event_str != '\0') {
// //                 *buf++ = *event_str++;
// //             }
// //             events &= ~mask;
// //             // If more events add ", "
// //             if (events) {
// //                 *buf++ = ',';
// //                 *buf++ = ' ';
// //             }
// //         }
// //     }
// //     *buf++ = '\0';
// // }

#include "pico/stdlib.h"

#define LED_PIN 15

int main() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, 1);
        gpio_put(17, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }
}
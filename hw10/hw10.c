#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

int main() {
    // Initialize USB serial
    stdio_init_all();

    // Initialize ADC
    adc_init();

    // Select ADC input 0 (GP26)
    adc_gpio_init(26);
    adc_select_input(0);

    while (true) {

        // Read 12-bit ADC (0–4095)
        uint16_t raw = adc_read();
        
        // Print as a single number (pgz will read this line)
        printf("%u\n", raw);

        // 20 ms delay (~50 Hz update rate)
        sleep_ms(20);
    }
}
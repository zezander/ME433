#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"

#define HX711_DT  16
#define HX711_SCK 17

#define MAX_SAMPLES 2000

int32_t  raw_data[MAX_SAMPLES];
float    filtered_data[MAX_SAMPLES];
uint32_t time_data[MAX_SAMPLES];

void hx711_init() {
    gpio_init(HX711_DT);
    gpio_set_dir(HX711_DT, GPIO_IN);

    gpio_init(HX711_SCK);
    gpio_set_dir(HX711_SCK, GPIO_OUT);
    gpio_put(HX711_SCK, 0);
}

int32_t read_hx711() {
    uint32_t raw = 0;

    // Wait until DT goes low (data ready)
    while (gpio_get(HX711_DT) == 1) {
        tight_loop_contents();
    }

    // Read 24 bits — sample DT after SCK goes high
    for (int i = 0; i < 24; i++) {
        gpio_put(HX711_SCK, 1);
        sleep_us(1);
        raw = (raw << 1) | gpio_get(HX711_DT);
        gpio_put(HX711_SCK, 0);
        sleep_us(1);
    }

    // 25th pulse — sets gain to 128 for next read
    gpio_put(HX711_SCK, 1);
    sleep_us(1);
    gpio_put(HX711_SCK, 0);
    sleep_us(1);

    // Sign-extend 24-bit two's complement to 32-bit signed int
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }

    return (int32_t)raw;
}

int main() {
    stdio_init_all();
    sleep_ms(3000);

    hx711_init();

    printf("HX711 ready\n");

    // Sanity check — print one reading before collecting
    int32_t test = read_hx711();
    printf("First raw reading: %ld\n", test);

    // Wait for the computer to send the number of samples
    int samples = 0;
    printf("Send number of samples:\n");
    scanf("%d", &samples);
    if (samples <= 0 || samples > MAX_SAMPLES) {
        samples = 500;
    }
    printf("Collecting %d samples...\n", samples);

    // Prime the IIR filter with a real reading
    float alpha = 0.1f;
    float filtered = (float)read_hx711();

    absolute_time_t start = get_absolute_time();

    // Collect silently — no printing during acquisition
    for (int i = 0; i < samples; i++) {
        int32_t raw = read_hx711();
        filtered = filtered + alpha * ((float)raw - filtered);

        raw_data[i]      = raw;
        filtered_data[i] = filtered;
        time_data[i]     = (uint32_t)(absolute_time_diff_us(start, get_absolute_time()) / 1000);
    }

    // Print all data after collection is complete
    printf("time_ms,raw,filtered\n");
    for (int i = 0; i < samples; i++) {
        printf("%lu,%ld,%.2f\n",
               (unsigned long)time_data[i],
               (long)raw_data[i],
               filtered_data[i]);
    }

    printf("Done\n");

    while (true) {
        sleep_ms(1000);
    }
}
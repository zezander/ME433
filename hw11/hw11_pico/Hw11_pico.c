#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/time.h"

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Pins can be changed, se e the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 4
#define UART_RX_PIN 5

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Pico started\r\n");

    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    while (1) {
        //printf("Pico alive\r\n");
        //sleep_ms(1000);

       if (uart_is_readable(UART_ID)) {
        char c = uart_getc(UART_ID);
        printf("%c", c);
        uart_putc(UART_ID, c);
        }

        // static absolute_time_t last_print;
        // if (absolute_time_diff_us(last_print, get_absolute_time()) > 1000000) {
        // printf("Pico alive\r\n");
        // last_print = get_absolute_time();
        // }
    }
}
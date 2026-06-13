#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define ADDR 0b0100000

void init_mpc();
bool read_mpc(int pin);
void write_mpc(int pin, uint8_t value);

int main()
{
stdio_init_all();

// I2C Initialisation. Using it at 400Khz.
i2c_init(I2C_PORT, 400*1000);
gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
uint8_t buf[2];
uint8_t read_reg[2];


// For more examples of I2C use see <https://github.com/raspberrypi/pico-examples/tree/master/i2c>

// Initialize the pins as GPIO outputs
init_mpc();

while (true) {
    while (read_mpc(0) == 1) {
        write_mpc(7, 1);
        write_mpc(6, 1);
        sleep_ms(200);
        write_mpc(6, 0);
        sleep_ms(200);
    }
    write_mpc(7, 0);
    write_mpc(6, 1);
    sleep_ms(200);
    write_mpc(6, 0);
    sleep_ms(200);
}

}

void init_mpc() {
uint8_t buf[2];
buf[0] = 0x0;
buf[1] = 0b00111111;
i2c_write_blocking(i2c_default, ADDR, buf, 2, false);
}

bool read_mpc(int pin) {
int out;
uint8_t read_reg[2];
read_reg[0] = 0x09;
uint8_t pin_bit = 0b1 << pin;
i2c_write_blocking(i2c_default, ADDR, &read_reg[0], 1, true);  // true to keep host control of bus
i2c_read_blocking(i2c_default, ADDR, &read_reg[1], 1, false);  // false - finished with bus
out = (pin_bit & read_reg[1]) >> pin;
return (bool) out;
}

void write_mpc(int pin, uint8_t value) {
uint8_t write_buf[2];
write_buf[0] = 0x0A;
i2c_write_blocking(i2c_default, ADDR, &write_buf[0], 1, true);  // true to keep host control of bus
i2c_read_blocking(i2c_default, ADDR, &write_buf[1], 1, false);  // false - finished with bus
if (value == 1) {
write_buf[1] = ((value << pin) | write_buf[1]);
i2c_write_blocking(i2c_default, ADDR, write_buf, 2, false);
}
if (value == 0) {
write_buf[1] = (write_buf[1] & (~(1 << pin)));
i2c_write_blocking(i2c_default, ADDR, write_buf, 2, false);
}
}
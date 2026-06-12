#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define LED 15          // heartbeat LED wired directly to Pico GPIO15
#define CHIP_ADDRESS 0x20  // A0/A1/A2 all grounded -> 0100000 = 0x20
#define IODIR 0x00
#define OLAT 0x0A
#define GPIO_REG 0x09

// function prototypes
void setPin(unsigned char reg, unsigned char value);
unsigned char readPin(unsigned char reg);
void heartbeat();

int main()
{
    stdio_init_all();

    // wait for USB serial connection so we don't miss early prints
    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }
    printf("=== Pico booted, USB connected ===\n");

    // I2C Initialisation at 400KHz
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    printf("I2C initialized on SDA=GPIO%d, SCL=GPIO%d\n", I2C_SDA, I2C_SCL);

    printf("Scanning I2C...\n");
    for (int addr = 0; addr < 128; addr++) {
        // skip reserved address ranges
        if ((addr & 0x78) == 0 || (addr & 0x78) == 0x78) continue;

        uint8_t rxdata;
        int result = i2c_read_blocking(I2C_PORT, addr, &rxdata, 1, false);
        if (result >= 0) {
            printf("Found device at 0x%02X\n", addr);
        }
    }

    // set up heartbeat LED on Pico
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    printf("Heartbeat LED initialized on GPIO%d\n", LED);

    // initial MCP23008 configuration
    // GP7 = output (LED), GP0-GP6 = inputs (button on GP0)
    printf("Configuring MCP23008 IODIR...\n");
    setPin(IODIR, 0b01111111); // GP7 output, everything else input

    // read back IODIR to verify it was set correctly
    unsigned char iodir_check = readPin(IODIR);
    printf("IODIR readback: 0x%02X (expected 0x7F)\n", iodir_check);
    if (iodir_check == 0x7F) {
        printf("MCP23008 config OK!\n");
    } else {
        printf("WARNING: IODIR mismatch - check wiring/address\n");
    }

    int loop_count = 0;

    while (true) {
        // heartbeat blink - shows Pico is alive and not stuck
        heartbeat();

        // read button state from MCP23008 GP0
        uint8_t gpio_val = readPin(GPIO_REG);
        printf("Loop %d | GPIO_REG=0x%02X | GP0(button)=%d\n",
               loop_count, gpio_val, gpio_val & 0x01);

        if (gpio_val & 0x01) {
            // GP0 high = button NOT pressed (pull-up)
            printf("  Button: not pressed -> LED off\n");
            setPin(OLAT, 0b00000000);
        } else {
            // GP0 low = button pressed
            printf("  Button: PRESSED -> LED on GP7\n");
            setPin(OLAT, 0b10000000);
        }

        loop_count++;
        sleep_ms(100);
    }
}

void setPin(unsigned char reg, unsigned char value) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    int result = i2c_write_blocking_until(I2C_PORT, CHIP_ADDRESS, buf, 2, false,
                                          make_timeout_time_ms(100));
    if (result == PICO_ERROR_GENERIC || result == PICO_ERROR_TIMEOUT) {
        printf("ERROR: i2c_write failed/timeout for reg=0x%02X\n", reg);
    }
}

unsigned char readPin(unsigned char reg) {
    uint8_t val = 0xFF;
    int wr = i2c_write_blocking_until(I2C_PORT, CHIP_ADDRESS, &reg, 1, true,
                                      make_timeout_time_ms(100));
    if (wr == PICO_ERROR_GENERIC || wr == PICO_ERROR_TIMEOUT) {
        printf("ERROR: i2c_write timeout reg=0x%02X\n", reg);
        return val;
    }
    int rd = i2c_read_blocking_until(I2C_PORT, CHIP_ADDRESS, &val, 1, false,
                                     make_timeout_time_ms(100));
    if (rd == PICO_ERROR_GENERIC || rd == PICO_ERROR_TIMEOUT) {
        printf("ERROR: i2c_read timeout reg=0x%02X\n", reg);
    }
    return val;
}

void heartbeat() {
    gpio_put(LED, 1);
    sleep_ms(50);
    gpio_put(LED, 0);
    sleep_ms(50);
}
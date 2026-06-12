// #include <stdio.h>
// #include "pico/stdlib.h"
// #include "hardware/i2c.h"

// // I2C defines
// #define I2C_PORT i2c0
// #define I2C_SDA 8
// #define I2C_SCL 9
// #define LED 15          // heartbeat LED wired directly to Pico GPIO15
// #define CHIP_ADDRESS 0x20
// #define IODIR 0x00
// #define OLAT 0x0A
// #define GPIO_REG 0x09

// // function prototypes
// void setPin(unsigned char reg, unsigned char value);
// unsigned char readPin(unsigned char reg);
// void heartbeat();

// int main()
// {
//     stdio_init_all();

//     // wait for USB serial connection so we don't miss early prints
//     while (!stdio_usb_connected()) {
//         tight_loop_contents();
//     }
//     printf("=== Pico booted, USB connected ===\n");

//     // I2C Initialisation at 400KHz
//     i2c_init(I2C_PORT, 400*1000);
//     gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
//     gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
//     gpio_pull_up(I2C_SDA);
//     gpio_pull_up(I2C_SCL);
//     printf("I2C initialized on SDA=GPIO%d, SCL=GPIO%d\n", I2C_SDA, I2C_SCL);

//     printf("Scanning I2C...\n");
//     // for (int addr = 0; addr < 128; addr++) {
//     //     uint8_t buf;
//     //     int result = i2c_read_blocking(I2C_PORT, addr, &buf, 1, false);
//     //     if (result >= 0) {
//     //         printf("Found device at address 0x%02X\n", addr);
//     //     }
//     // }
//     for (int addr = 0; addr < 128; addr++) {
//     int result = i2c_write_blocking(I2C_PORT, addr, NULL, 0, false);
//     if (result >= 0) {
//         printf("Found device at 0x%02X\n", addr);
//     }
//     // set up heartbeat LED on Pico
//     gpio_init(LED);
//     gpio_set_dir(LED, GPIO_OUT);
//     printf("Heartbeat LED initialized on GPIO%d\n", LED);

//     // initial MCP23008 configuration
//     // GP7 = output (LED), GP0 = input (button), rest = inputs
//     printf("Configuring MCP23008 IODIR...\n");
//     setPin(IODIR, 0b01111110); // FIXED: GP7 output, GP0 input
    
//     // read back IODIR to verify it was set correctly
//     unsigned char iodir_check = readPin(IODIR);
//     printf("IODIR readback: 0x%02X (expected 0x7E)\n", iodir_check);
//     if (iodir_check == 0x7E) {
//         printf("MCP23008 config OK!\n");
//     } else {
//         printf("WARNING: IODIR mismatch - check wiring/address\n");
//     }

//     int loop_count = 0;

//     while (true) {
//         // heartbeat blink - shows Pico is alive and not stuck
//         heartbeat();

//         // read button state from MCP23008 GP0
//         uint8_t gpio_val = readPin(GPIO_REG);
//         printf("Loop %d | GPIO_REG=0x%02X | GP0(button)=%d\n",
//                loop_count, gpio_val, gpio_val & 0x01);

//         if (gpio_val & 0x01) {
//             // GP0 high = button NOT pressed (pull-up)
//             printf("  Button: not pressed -> LED off\n");
//             setPin(OLAT, 0b00000000);
//         } else {
//             // GP0 low = button pressed
//             printf("  Button: PRESSED -> LED on GP7\n");
//             setPin(OLAT, 0b10000000);
//         }

//         loop_count++;
//         sleep_ms(100);
//     }
// }

// // void setPin(unsigned char reg, unsigned char value) {
// //     uint8_t buf[2];
// //     buf[0] = reg;
// //     buf[1] = value;
// //     int result = i2c_write_blocking(I2C_PORT, CHIP_ADDRESS, buf, 2, false);
// //     if (result == PICO_ERROR_GENERIC) {
// //         printf("  ERROR: i2c_write failed for reg=0x%02X (is MCP23008 connected?)\n", reg);
// //     }
// // }
// void setPin(unsigned char reg, unsigned char value) {
//     uint8_t buf[2];
//     buf[0] = reg;
//     buf[1] = value;
//     int result = i2c_write_blocking_until(I2C_PORT, CHIP_ADDRESS, buf, 2, false, 
//                                           make_timeout_time_ms(100));
//     if (result == PICO_ERROR_GENERIC || result == PICO_ERROR_TIMEOUT) {
//         printf("ERROR: i2c_write failed/timeout for reg=0x%02X\n", reg);
//     }
// }

// // unsigned char readPin(unsigned char reg) {
// //     uint8_t val = 0xFF; // default to 0xFF so a failed read is obvious
// //     int write_result = i2c_write_blocking(I2C_PORT, CHIP_ADDRESS, &reg, 1, true);
// //     if (write_result == PICO_ERROR_GENERIC) {
// //         printf("  ERROR: i2c_write (set reg) failed for reg=0x%02X\n", reg);
// //         return val;
// //     }
// //     int read_result = i2c_read_blocking(I2C_PORT, CHIP_ADDRESS, &val, 1, false);
// //     if (read_result == PICO_ERROR_GENERIC) {
// //         printf("  ERROR: i2c_read failed for reg=0x%02X\n", reg);
// //     }
// //     return val;
// // }
// unsigned char readPin(unsigned char reg) {
//     uint8_t val = 0xFF;
//     int wr = i2c_write_blocking_until(I2C_PORT, CHIP_ADDRESS, &reg, 1, true,
//                                       make_timeout_time_ms(100));
//     if (wr == PICO_ERROR_GENERIC || wr == PICO_ERROR_TIMEOUT) {
//         printf("ERROR: i2c_write timeout reg=0x%02X\n", reg);
//         return val;
//     }
//     int rd = i2c_read_blocking_until(I2C_PORT, CHIP_ADDRESS, &val, 1, false,
//                                      make_timeout_time_ms(100));
//     if (rd == PICO_ERROR_GENERIC || rd == PICO_ERROR_TIMEOUT) {
//         printf("ERROR: i2c_read timeout reg=0x%02X\n", reg);
//     }
//     return val;
// }
// void heartbeat() {
//     gpio_put(LED, 1);
//     sleep_ms(50);
//     gpio_put(LED, 0);
//     sleep_ms(50);
// }

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ===================== I2C CONFIG =====================
#define I2C_PORT i2c0
#define I2C_SDA 1   // GPIO1
#define I2C_SCL 2   // GPIO2

#define LED 15      // Pico heartbeat LED

// MCP23008
#define CHIP_ADDRESS 0x20

#define IODIR    0x00
#define GPIO_REG 0x09
#define OLAT     0x0A

// ===================== PROTOTYPES =====================
void setReg(uint8_t reg, uint8_t val);
uint8_t readReg(uint8_t reg);
void heartbeat(void);

// ===================== MAIN =====================
int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }

    printf("=== Booting MCP23008 test ===\n");

    // ---------------- I2C INIT ----------------
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    sleep_ms(100);

    printf("I2C0 on SDA=GP%d SCL=GP%d\n", I2C_SDA, I2C_SCL);

    // ---------------- HEARTBEAT LED ----------------
    uint8_t who = 0x00;
    int ret = i2c_write_blocking(i2c0, 0x20, &who, 0, false);

    printf("MCP23008 ACK test result: %d\n", ret);
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    // ---------------- PROPER I2C SCAN ----------------
    printf("Scanning I2C bus...\n");
    for (int addr = 0; addr < 128; addr++) {
        int ack = i2c_write_blocking(I2C_PORT, addr, NULL, 0, false);
        if (ack >= 0) {
            printf("Found device at 0x%02X\n", addr);
        }
    }

    // ---------------- MCP SETUP ----------------
    printf("Configuring MCP23008...\n");

    // GP7 = output (LED), GP0 = input (button)
    setReg(IODIR, 0b01111111);

    sleep_ms(10);

    uint8_t test = readReg(IODIR);
    printf("IODIR readback: 0x%02X\n", test);

    if (test != 0b01111111) {
        printf("WARNING: MCP not responding correctly\n");
    }

    int loop = 0;

    // ================= MAIN LOOP =================
    while (1) {
        heartbeat();

        uint8_t gpio = readReg(GPIO_REG);

        printf("Loop %d | GPIO=0x%02X | GP0=%d\n",
               loop, gpio, gpio & 0x01);

        if (gpio & 0x01) {
            // not pressed (pull-up)
            setReg(GPIO_REG, 0x00);
        } else {
            // pressed
            setReg(GPIO_REG, 0x80); // GP7 high
        }

        loop++;
        sleep_ms(100);
    }
}

// ===================== I2C WRITE =====================
void setReg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};

    int ret = i2c_write_blocking(I2C_PORT, CHIP_ADDRESS, buf, 2, false);

    if (ret < 0) {
        printf("I2C WRITE FAIL reg=0x%02X\n", reg);
    }
}

// ===================== I2C READ =====================
uint8_t readReg(uint8_t reg) {
    uint8_t val = 0xFF;

    int ret = i2c_write_blocking(I2C_PORT, CHIP_ADDRESS,
                                 &reg, 1, true);

    if (ret < 0) {
        printf("I2C REG SET FAIL reg=0x%02X\n", reg);
        return val;
    }

    ret = i2c_read_blocking(I2C_PORT, CHIP_ADDRESS,
                            &val, 1, false);

    if (ret < 0) {
        printf("I2C READ FAIL reg=0x%02X\n", reg);
        return 0xFF;
    }

    return val;
}

// ===================== HEARTBEAT =====================
void heartbeat() {
    gpio_put(LED, 1);
    sleep_ms(50);
    gpio_put(LED, 0);
    sleep_ms(50);
}
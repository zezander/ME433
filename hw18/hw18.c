#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

// ── AS5600 magnetic encoder ──────────────────────────────────────────────────
#define I2C_PORT        i2c0
#define SDA_PIN         4
#define SCL_PIN         5
#define AS5600_ADDR     0x36
#define AS5600_STATUS   0x0B   // bits: [5]=MD [4]=ML [3]=MH
#define AS5600_AGC      0x1A   // automatic gain control (~128 = ideal)
#define AS5600_ANGLE_H  0x0E   // filtered angle high nibble [11:8]
// 0x0F is angle low byte [7:0] — read automatically as part of 2-byte burst

// ── DRV8833 motor (PWM) ──────────────────────────────────────────────────────
#define PWM_PIN1  14
#define PWM_PIN2  15
#define MAX_PWM   65535u

// ── HX711 force sensor ───────────────────────────────────────────────────────
#define HX711_DT  16
#define HX711_SCK 17

// ── Haptic geometry ──────────────────────────────────────────────────────────
#define CENTER_DEG  173.0f   // encoder degrees at rest/center
#define RANGE_DEG    45.0f   // +/- degrees from center before clamp

// ── Spring stiffness (tune this) ─────────────────────────────────────────────
#define K_SPRING  0.3f       // 0 = no force, 1 = full PWM at edge of range

// ── Diagnostic print interval ────────────────────────────────────────────────
#define DIAG_INTERVAL_MS  500   // print magnet status every 500 ms


// ─────────────────────────────────────────────────────────────────────────────
// Motor: u in [-1, +1].  Positive u = forward, negative = reverse.
// One PWM pin is driven, the other held low.
// u == 0  →  both HIGH  →  DRV8833 brake/coast mode.
// ─────────────────────────────────────────────────────────────────────────────
void set_motor(float u) {
    if (u >  1.0f) u =  1.0f;
    if (u < -1.0f) u = -1.0f;

    uint16_t pwm = (uint16_t)(fabsf(u) * (float)MAX_PWM);

    if (u > 0.0f) {
        pwm_set_gpio_level(PWM_PIN1, pwm);
        pwm_set_gpio_level(PWM_PIN2, 0);
    } else if (u < 0.0f) {
        pwm_set_gpio_level(PWM_PIN1, 0);
        pwm_set_gpio_level(PWM_PIN2, pwm);
    } else {
        // brake: both high
        pwm_set_gpio_level(PWM_PIN1, MAX_PWM);
        pwm_set_gpio_level(PWM_PIN2, MAX_PWM);
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// AS5600 helpers
// NOTE: nostop=false (STOP + START) is used instead of a repeated-start
// because some AS5600 units are unreliable with repeated-start at 400 kHz.
// ─────────────────────────────────────────────────────────────────────────────

// Read a single byte from an AS5600 register.
static uint8_t as5600_read_byte(uint8_t reg) {
    uint8_t val = 0;
    i2c_write_blocking(I2C_PORT, AS5600_ADDR, &reg, 1, false); // STOP after write
    i2c_read_blocking (I2C_PORT, AS5600_ADDR, &val, 1, false);
    return val;
}

// Read the 12-bit filtered angle (registers 0x0E–0x0F).
// Returns raw value 0–4095, or 0xFFFF on I2C error.
uint16_t read_as5600_angle(void) {
    uint8_t reg = AS5600_ANGLE_H;
    uint8_t data[2] = {0, 0};

    // Write register address, send STOP, then read 2 bytes.
    int wr = i2c_write_blocking(I2C_PORT, AS5600_ADDR, &reg, 1, false);
    if (wr < 0) return 0xFFFF;

    int rd = i2c_read_blocking(I2C_PORT, AS5600_ADDR, data, 2, false);
    if (rd < 0) return 0xFFFF;

    return ((uint16_t)(data[0] & 0x0F) << 8) | data[1];
}

// Read and decode the STATUS register.
// Fills the three flag pointers (1 = asserted, 0 = not).
void read_as5600_status(bool *md, bool *ml, bool *mh) {
    uint8_t s = as5600_read_byte(AS5600_STATUS);
    *md = (s >> 5) & 1;  // Magnet Detected
    *ml = (s >> 4) & 1;  // Magnet too weak / too far
    *mh = (s >> 3) & 1;  // Magnet too strong / too close
}

// Read AGC register — ideal is ~128 (mid-range).
// Low value (~0–50)   = magnet too strong/close.
// High value (~200–255) = magnet too weak/far or absent.
uint8_t read_as5600_agc(void) {
    return as5600_read_byte(AS5600_AGC);
}

// Print a human-readable magnet diagnostic over USB serial.
void print_as5600_diag(void) {
    bool md, ml, mh;
    read_as5600_status(&md, &ml, &mh);
    uint8_t agc = read_as5600_agc();

    printf("[AS5600 DIAG] STATUS: MD=%d ML=%d MH=%d | AGC=%u",
           (int)md, (int)ml, (int)mh, (unsigned)agc);

    if (!md) {
        printf(" ← NO MAGNET DETECTED — angle is invalid!");
    } else if (ml) {
        printf(" ← magnet too weak/far — move magnet closer");
    } else if (mh) {
        printf(" ← magnet too strong/close — move magnet farther");
    } else {
        printf(" ← magnet OK");
    }
    printf("\n");
}


// ─────────────────────────────────────────────────────────────────────────────
// HX711: waits for data-ready (DT low), clocks out 24 bits, sends 25th pulse
// to set gain=128 for next read, then sign-extends to int32.
// ─────────────────────────────────────────────────────────────────────────────
int32_t read_hx711(void) {
    uint32_t raw = 0;

    // Wait until DT goes low (data ready).
    while (gpio_get(HX711_DT) == 1) {
        tight_loop_contents();
    }

    // Clock out 24 bits — sample DT while SCK is HIGH.
    for (int i = 0; i < 24; i++) {
        gpio_put(HX711_SCK, 1);
        sleep_us(1);
        raw = (raw << 1) | gpio_get(HX711_DT);
        gpio_put(HX711_SCK, 0);
        sleep_us(1);
    }

    // 25th pulse — selects gain=128 for next conversion.
    gpio_put(HX711_SCK, 1);
    sleep_us(1);
    gpio_put(HX711_SCK, 0);
    sleep_us(1);

    // Sign-extend 24-bit two's complement → int32.
    if (raw & 0x800000u) {
        raw |= 0xFF000000u;
    }

    return (int32_t)raw;
}


// ─────────────────────────────────────────────────────────────────────────────
int main(void) {
    stdio_init_all();
    sleep_ms(2000);  // let USB serial enumerate

    // ── I2C for AS5600 ───────────────────────────────────────────────────────
    // 100 kHz is more reliable than 400 kHz, especially with only internal
    // pull-ups.  Add external 4.7 kΩ pull-ups to 3V3 if you want 400 kHz.
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // ── I2C bus scan ─────────────────────────────────────────────────────────
    printf("Scanning I2C bus at 100 kHz...\n");
    bool found_encoder = false;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        uint8_t buf;
        int ret = i2c_read_blocking(I2C_PORT, addr, &buf, 1, false);
        if (ret >= 0) {
            printf("  Found device at 0x%02X%s\n",
                   addr,
                   addr == AS5600_ADDR ? "  ← AS5600" : "");
            if (addr == AS5600_ADDR) found_encoder = true;
        }
    }
    if (!found_encoder) {
        printf("  *** AS5600 NOT found at 0x36 — check wiring, power, pull-ups ***\n");
    }
    printf("Scan done.\n\n");

    // ── Initial AS5600 magnet diagnostic ────────────────────────────────────
    print_as5600_diag();

    // ── HX711 GPIO ───────────────────────────────────────────────────────────
    gpio_init(HX711_DT);
    gpio_set_dir(HX711_DT, GPIO_IN);
    gpio_init(HX711_SCK);
    gpio_set_dir(HX711_SCK, GPIO_OUT);
    gpio_put(HX711_SCK, 0);

    // ── PWM for DRV8833 ──────────────────────────────────────────────────────
    // GP14 = slice 7 channel A,  GP15 = slice 7 channel B
    gpio_set_function(PWM_PIN1, GPIO_FUNC_PWM);
    gpio_set_function(PWM_PIN2, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PWM_PIN1);
    pwm_set_wrap(slice, MAX_PWM);
    pwm_set_enabled(slice, true);

    set_motor(0.0f);  // start with motor off

    // ── CSV header for Python ─────────────────────────────────────────────────
    printf("angle_deg,load_raw,x,u\n");

    // ── IIR filter state for force ────────────────────────────────────────────
    float filtered_force = (float)read_hx711();
    const float alpha = 0.1f;

    // ── Diagnostic timer ──────────────────────────────────────────────────────
    uint32_t last_diag_ms = 0;

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (true) {

        // Periodic magnet-health diagnostic (does not stall the loop)
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if (now_ms - last_diag_ms >= DIAG_INTERVAL_MS) {
            last_diag_ms = now_ms;
            print_as5600_diag();
        }

        // 1. Read position
        uint16_t angle_raw = read_as5600_angle();

        // Detect I2C failure or missing magnet
        if (angle_raw == 0xFFFF) {
            printf("ERR: AS5600 I2C read failed\n");
            set_motor(0.0f);
            sleep_ms(10);
            continue;
        }

        float angle_deg = (float)angle_raw * 360.0f / 4096.0f;

        // 2. Normalize position: x = -1 at left limit, 0 at center, +1 at right
        float x = (angle_deg - CENTER_DEG) / RANGE_DEG;
        if (x >  1.0f) x =  1.0f;
        if (x < -1.0f) x = -1.0f;

        // 3. Read and filter force
        int32_t load_raw = read_hx711();
        filtered_force += alpha * ((float)load_raw - filtered_force);

        // 4. Haptic law: spring restoring force pulls back toward center.
        //    Negative sign: if x > 0 (paddle pushed right), push back left.
        float u = -K_SPRING * x;

        // 5. Command motor
        set_motor(u);

        // 6. Send data to Python (CSV: angle_deg, load_raw, x, u)
        printf("%.2f,%ld,%.3f,%.3f\n", angle_deg, (long)load_raw, x, u);

        sleep_ms(10);
    }
}
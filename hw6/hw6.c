/*
 * The MIT License (MIT)
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * [license text unchanged]
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

//--------------------------------------------------------------------+
// IMU setup
//--------------------------------------------------------------------+
#define I2C_PORT     i2c0
#define SDA_PIN      0
#define SCL_PIN      1
#define MPU_ADDR     0x68

#define ACCEL_CONFIG 0x1C
#define GYRO_CONFIG  0x1B
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B

//--------------------------------------------------------------------+
// Mode control
// GP14 = mode LED  (moved off GP15 to avoid board LED conflict)
// GP16 = mode button, wired to GND, internal pull-up enabled
//--------------------------------------------------------------------+
#define MODE_BUTTON_PIN 16
#define MODE_LED_PIN    15   

static bool  remote_mode       = false;
static bool  last_button_state = true;
static float circle_t          = 0.0f;

void init_imu(void);
void read_imu(float *accel, float *gyro, float *temp);

//--------------------------------------------------------------------+
// Blink intervals (board LED only, not the mode LED)
//--------------------------------------------------------------------+
enum {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED     = 1000,
  BLINK_SUSPENDED   = 2500,
};
static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);

//--------------------------------------------------------------------+
// MAIN
//--------------------------------------------------------------------+
int main(void)
{
  board_init();
  sleep_ms(2000);

  tud_init(BOARD_TUD_RHPORT);
  if (board_init_after_tusb) board_init_after_tusb();

  // I2C for MPU6050
  i2c_init(I2C_PORT, 400000);
  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(SDA_PIN);
  gpio_pull_up(SCL_PIN);
  init_imu();

  // Mode button: GP16 → button → GND, pull-up so idle = HIGH
  gpio_init(MODE_BUTTON_PIN);
  gpio_set_dir(MODE_BUTTON_PIN, GPIO_IN);
  gpio_pull_up(MODE_BUTTON_PIN);

  // Mode LED: GP14 → 330R → LED → GND
  gpio_init(MODE_LED_PIN);
  gpio_set_dir(MODE_LED_PIN, GPIO_OUT);
  gpio_put(MODE_LED_PIN, 0);

  // Sanity-check: blink the mode LED twice on startup so you know the pin works
  for (int i = 0; i < 2; i++) {
    gpio_put(MODE_LED_PIN, 1); sleep_ms(200);
    gpio_put(MODE_LED_PIN, 0); sleep_ms(200);
  }

  printf("IMU mouse ready. Tilt board to move cursor. Button on GP16 toggles circle mode.\n");

  while (1) {
    tud_task();
    led_blinking_task();
    hid_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+
void tud_mount_cb(void)  { blink_interval_ms = BLINK_MOUNTED; }
void tud_umount_cb(void) { blink_interval_ms = BLINK_NOT_MOUNTED; }

void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// HID: send only mouse report
//--------------------------------------------------------------------+
static void send_hid_report(void)
{
  if (!tud_hid_ready()) return;

  int8_t dx = 0, dy = 0;

  // Button: falling edge = toggle mode
  bool current_button_state = gpio_get(MODE_BUTTON_PIN);
  if (last_button_state == true && current_button_state == false) {
    remote_mode = !remote_mode;
    printf("Mode toggled: %s\n", remote_mode ? "CIRCLE" : "IMU");
    sleep_ms(50); // debounce
  }
  last_button_state = current_button_state;

  // LED: on = circle mode, off = IMU mode
  gpio_put(MODE_LED_PIN, remote_mode ? 1 : 0);

  if (!remote_mode) {
    // -------- IMU mouse mode --------
    float accel[3], gyro[3], temp;
    read_imu(accel, gyro, &temp);

    // X tilt → cursor left/right
    if      (accel[0] >  0.5f) dx =  5;
    else if (accel[0] >  0.2f) dx =  2;
    else if (accel[0] < -0.5f) dx = -5;
    else if (accel[0] < -0.2f) dx = -2;

    // Y tilt → cursor up/down (inverted)
    if      (accel[1] >  0.5f) dy = -5;
    else if (accel[1] >  0.2f) dy = -2;
    else if (accel[1] < -0.5f) dy =  5;
    else if (accel[1] < -0.2f) dy =  2;

    // Only print when there's actual movement — not every cycle
    if (dx != 0 || dy != 0) {
      printf("Mouse move: dx=%d dy=%d  (ax=%.2f ay=%.2f)\n", dx, dy, accel[0], accel[1]);
    }

  } else {
    // -------- Circle mode --------
    dx = (int8_t)(30.0f * cosf(circle_t));
    dy = (int8_t)(30.0f * sinf(circle_t));
    circle_t += 0.08f;
    if (circle_t > 6.28318f) circle_t = 0.0f;
  }

  tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, dx, dy, 0, 0);
}

//--------------------------------------------------------------------+
// HID task: 10 ms poll
//--------------------------------------------------------------------+
void hid_task(void)
{
  const uint32_t interval_ms = 10;
  static uint32_t start_ms   = 0;

  if (board_millis() - start_ms < interval_ms) return;
  start_ms += interval_ms;

  if (tud_suspended()) {
    tud_remote_wakeup();
  } else {
    send_hid_report();
  }
}

//--------------------------------------------------------------------+
// Report complete: intentionally empty — no chaining
//--------------------------------------------------------------------+
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance; (void) report; (void) len;
}

//--------------------------------------------------------------------+
// Required stubs
//--------------------------------------------------------------------+
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen)
{
  (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance; (void) report_id; (void) report_type; (void) buffer; (void) bufsize;
}

//--------------------------------------------------------------------+
// Board LED blink task
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state     = false;

  if (!blink_interval_ms) return;
  if (board_millis() - start_ms < blink_interval_ms) return;
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state;
}

//--------------------------------------------------------------------+
// IMU functions
//--------------------------------------------------------------------+
void init_imu(void)
{
  uint8_t buf[2];

  buf[0] = PWR_MGMT_1;   buf[1] = 0x00;
  i2c_write_blocking(i2c0, MPU_ADDR, buf, 2, false);

  buf[0] = ACCEL_CONFIG; buf[1] = 0x00;  // ±2g
  i2c_write_blocking(i2c0, MPU_ADDR, buf, 2, false);

  buf[0] = GYRO_CONFIG;  buf[1] = 0x18;  // ±2000 dps
  i2c_write_blocking(i2c0, MPU_ADDR, buf, 2, false);
}

void read_imu(float *accel, float *gyro, float *temp)
{
  uint8_t reg = ACCEL_XOUT_H;
  uint8_t data[14];

  i2c_write_blocking(i2c0, MPU_ADDR, &reg, 1, true);
  i2c_read_blocking (i2c0, MPU_ADDR, data, 14, false);

  int16_t ax       = (data[0]  << 8) | data[1];
  int16_t ay       = (data[2]  << 8) | data[3];
  int16_t az       = (data[4]  << 8) | data[5];
  int16_t gx       = (data[8]  << 8) | data[9];
  int16_t gy       = (data[10] << 8) | data[11];
  int16_t gz       = (data[12] << 8) | data[13];
  int16_t temp_raw = (data[6]  << 8) | data[7];

  accel[0] = ax * 0.000061f;
  accel[1] = ay * 0.000061f;
  accel[2] = az * 0.000061f;

  gyro[0] = gx * 0.00763f;
  gyro[1] = gy * 0.00763f;
  gyro[2] = gz * 0.00763f;

  *temp = temp_raw / 340.0f + 36.53f;
}
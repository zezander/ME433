#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PWMPIN 15


// your existing setServo function — unchanged
// void setServo(float angle){
//     pwm_set_gpio_level(PWMPIN, (int)((0.05 + (angle/180.0)*0.05)*60000));
// }
void setServo(float angle) {
    // 1MHz clock, so 1 count = 1 microsecond
    // 0°   = 1000us (1ms)
    // 90°  = 1500us (1.5ms)
    // 180° = 2000us (2ms)
    uint level = (uint)(1000 + (angle / 180.0f) * 1000);
    pwm_set_gpio_level(PWMPIN, level);
}

void shake() {
    for (int i = 0; i < 4; i++) {
        setServo(70);   // shake left
        sleep_ms(60);
        setServo(110);  // shake right
        sleep_ms(60);
    }
    setServo(90);       // return to center
}

int main() {
    stdio_init_all();
    // your existing PWM setup — unchanged
    gpio_set_function(PWMPIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWMPIN);
   // PWM setup - gives exactly 50Hz
    pwm_set_clkdiv(slice_num, 125);       // 125MHz / 125 = 1MHz
    pwm_set_wrap(slice_num, 20000);        // 1MHz / 20000 = 50Hz
    pwm_set_enabled(slice_num, true);

    // center servo on startup
    setServo(0);
    sleep_ms(1000);
    setServo(180);
    sleep_ms(1000);
    setServo(90);
    printf("Ready. Type 0 and hit enter to shake servo.\n");

    char buf[32];
    int buf_i = 0;

    while (true) {
        int c = getchar_timeout_us(100000);

        if (c == PICO_ERROR_TIMEOUT) continue;

        if (c == '\n' || c == '\r') {
            if (buf_i > 0) {
                buf[buf_i] = '\0';
                int num = atoi(buf);
                if (num == 0) {
                    printf("Shaking!\n");
                    shake();
                    printf("Done.\n");
                } else {
                    printf("Got %d — only servo 0 is wired for this test.\n", num);
                }
                buf_i = 0;
            }
        } else {
            if (buf_i < (int)(sizeof(buf) - 1)) {
                buf[buf_i++] = (char)c;
            }
        }
    }
}

#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
// #include "pico/binary_info.h"
#include "hardware/spi.h"
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS 15
#define PIN_SCK 18
#define PIN_MOSI 19

void spi_init(void);
static inline void cs_select(uint cs_pin);
static inline void cs_deselect(uint cs_pin);
void writeDAC(int channel, float volts);



int main()
{
    stdio_init_all();
    spi_init();
    float t=0;
    float v = 0;

    while (true) {
        for (int i=0;i<200;i++){
            v = 1.65*sin(2*t*2.0*math.pi) + 1.65;
            writeDAC(0,v);
            v = fabs(3.3*(fmod(2*t,2)-1));
            writeDAC(1,v);
            t=t+0.002;
            sleep_ms(2);
        }
        printf("Data written!\r\n");
        //sleep_ms(0.01);
    }
}
void spi_init(void){
    spi_init(SPI_PORT, 1000*1000); // the baud, or bits per second
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_dir(PIN_CS,GPIO_OUT);
    gpio_put(PIN_CS,1);
}

static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}

void writeDAC(int channel, float volts){
    uint8_t data[2];
    int len = 2;
    if (volts < 0){
        volts = 0;
    }
    if (volts > 3.3){
        volts = 3.3;
    }
    uint16_t v_bin = (uint16_t)((volts / 3.3) * 1023.0);
    data[0] = 0;
    data[1] = 0;
    data[0] = data[0] | (channel<<7);
    data[0] = data[0] | (0b111<<4);
    data[0] = data[0] | (v_bin>>6);
    data[1] = data[1] | (v_bin << 2);
    //data[0] = 0b01111000;
    //data[1] = 0b00000000;
    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, len); // where data is a uint8_t array with length len
    cs_deselect(PIN_CS);
}

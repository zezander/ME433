#include <stdio.h>
#include "pico/stdlib.h"
#include <math.h>
#include "hardware/spi.h"
#include "hardware/adc.h"

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define PIN_ADC_A 26   // GP26 = ADC0, connected to VOUTA (sine)
#define PIN_ADC_B 27   // GP27 = ADC1, connected to VOUTB (triangle)

static inline void cs_select(uint cs_pin);
static inline void cs_deselect(uint cs_pin);
void writeDac(int channel, float voltage);

int main()
{
    stdio_init_all();
    sleep_ms(2000);  // give time for USB to connect
    printf("Pico started\n");
    // init the SPI pins and peripherals
    spi_init(spi_default, 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    // init the CS pin
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // init ADC for reading VOUTA and VOUTB
    adc_gpio_init(PIN_ADC_A);
    adc_gpio_init(PIN_ADC_B);

    // explicitly disable pull-ups/downs on ADC pins
    gpio_disable_pulls(PIN_ADC_A);
    gpio_disable_pulls(PIN_ADC_B);
    float t = 0;

//     while (true) {
//         // channel A: 2Hz sine wave, 0 to 3.3V
//         float voltageA = 1.65 * (sin(4 * M_PI * t) + 1);

//         // channel B: 1Hz triangle wave, 0 to 3.3V
//         float period = 1.0;
//         float phase = fmod(t, period) / period;
//         float voltageB;
//         if (phase < 0.5) {
//             voltageB = phase * 2.0 * 3.3;
//         } else {
//             voltageB = 3.3 - 6.6 * (phase - 0.5);
//         }

//         writeDac(0, voltageA);
//         writeDac(1, voltageB);

//         // read actual output voltages back from the DAC pins via ADC
//         adc_select_input(0);  // ADC0 = GP26 = VOUTA
//         float measuredA = adc_read() * 3.3f / 4095.0f;

//         adc_select_input(1);  // ADC1 = GP27 = VOUTB
//         float measuredB = adc_read() * 3.3f / 4095.0f;

//         // send timestamp, reference (computed) and measured (ADC) voltages
//         // format: t, refA, refB, measA, measB
//         printf("%f,%f,%f,%f,%f\n", t, voltageA, voltageB, measuredA, measuredB);

//         sleep_ms(10);
//         t += 0.01;
//     }
// }
    while (true) {
        float voltageA = 1.65 * (sin(4 * M_PI * t) + 1);

        float period = 1.0;
        float phase = fmod(t, period) / period;
        float voltageB;
        if (phase < 0.5) {
            voltageB = phase * 2.0 * 3.3;
        } else {
            voltageB = 3.3 - 6.6 * (phase - 0.5);
        }

        writeDac(0, voltageA);
        writeDac(1, voltageB);

        adc_select_input(0);
        uint16_t rawA = adc_read();

        adc_select_input(1);
        uint16_t rawB = adc_read();

        printf("rawA: %d, rawB: %d\n", rawA, rawB);

        sleep_ms(100);
        t += 0.01;
    }
}

static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop");
}

void writeDac(int channel, float voltage) {
    if (voltage < 0) voltage = 0;
    if (voltage > 3.3) voltage = 3.3;

    uint8_t data[2];
    uint16_t theV = (uint16_t)((voltage / 3.3f) * 1023);

    data[0] = 0;
    data[0] |= (channel << 7);
    data[0] |= 0x30;
    data[0] |= (theV >> 6);
    data[1] = (theV << 2) & 0xFC;

    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, 2);
    cs_deselect(PIN_CS);
}
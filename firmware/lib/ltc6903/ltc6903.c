#include "ltc6903.h"

static const uint16_t CNF = 0b10; // Enable non-inverting output
static uint16_t bitmap;

void ltc_initialize(ltc_handle *h) {
    bitmap = CNF;
    gpio_init(h->cs);
    gpio_init(h->oe);
    gpio_set_dir(h->cs, GPIO_OUT);
    gpio_set_dir(h->oe, GPIO_OUT);
    gpio_put(h->oe, 1);
    gpio_put(h->cs, 1);
}

void ltc_uninitialize(ltc_handle *h) {
    gpio_put(h->cs, 1);
    gpio_deinit(h->cs);
    gpio_put(h->oe, 0);
    gpio_deinit(h->oe);
}

static void ltc_write(ltc_handle *h) {
    gpio_put(h->cs, 0);
    uint8_t data[2];
    data[1] = (uint8_t)bitmap;
    data[0] = (uint8_t)(bitmap >> 8);
    spi_write_blocking(h->spi, data, 2);
    gpio_put(h->cs, 1);
}

void ltc_set_freq(ltc_handle *h, uint32_t freq) {
    uint16_t oct = 3.322f * log10(freq / 1039);
    uint16_t dac = round(2048 - (2078 * pow(2, 10 + oct)) / freq);
    bitmap = (oct << 12) | (dac << 2) | CNF;
    ltc_write(h);
}

void ltc_output_enable(ltc_handle *h, bool enable) {
    gpio_put(h->oe, enable);
}

void ltc_low_power(ltc_handle *h, bool enable) {
    gpio_put(h->cs, 0);
    if(enable) {
        bitmap |= 0b11;
    } else {
        bitmap &= 0b1111111111111100;
        bitmap |= CNF;
    }
    ltc_write(h);
}
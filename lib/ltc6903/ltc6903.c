#include "ltc6903.h"

static void ltc_initialize(ltc_handle *h) {
    gpio_init(h->cs);
    gpio_init(h->oe);
    gpio_set_dir(h->cs, GPIO_OUT);
    gpio_set_dir(h->oe, GPIO_OUT);
    gpio_put(h->oe, 0);
    gpio_put(h->cs, 1);
}

static void ltc_uninitialize(ltc_handle *h) {
    gpio_put(h->cs, 1);
    gpio_deinit(h->cs);
    gpio_put(h->oe, 0);
    gpio_deinit(h->oe);
}

static void ltc_write(ltc_handle *h, uint16_t oct, uint16_t dac) {
    uint16_t bitmap = (oct << 12) | (dac << 2);
    gpio_put(h->cs, 0);
    spi_write16_blocking(h->spi, &bitmap, 1);
    gpio_put(h->cs, 1);
}

static void ltc_set_freq(ltc_handle *h, uint32_t freq) {
    uint16_t oct = 3.322f * log10(freq / 1039);
    uint16_t dac = round(2048 - (2078 * pow(2, 10 + oct)) / freq);
    ltc_write(h, oct, dac);
}

static void ltc_output_enable(ltc_handle *h, bool enable) {
    gpio_put(h->oe, enable);
}
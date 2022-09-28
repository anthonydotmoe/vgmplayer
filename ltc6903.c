#include "ltc6903.h"

void ltc_write(uint16_t oct, uint16_t dac) {
    uint16_t bitmap = (oct << 12) | (dac << 2);
    spi_write16_blocking(spi0, &bitmap, 1);
}

void ltc_set_freq(uint32_t freq) {
    uint16_t oct = 3.322f * log10(freq / 1039);
    uint16_t dac = round(2048 - (2078 * pow(2, 10 + oct)) / freq);
    ltc_write(oct, dac);
}
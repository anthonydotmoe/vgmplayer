#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"

typedef struct ltc_handle {
    spi_inst_t *spi;
    uint cs;
    uint oe;
} ltc_handle;

void ltc_initialize(ltc_handle *h);
void ltc_uninitialize(ltc_handle *h);

void ltc_set_freq(ltc_handle *h, const uint32_t freq);
void ltc_output_enable(ltc_handle *h, bool enable);
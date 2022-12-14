#pragma once

#include <stdio.h>

#include "ff.h"
#include "hw_config.h"
#include "sd_spi.h"

#include "hardware/pio.h"
#include "hardware/timer.h"
#include "ym2151.pio.h"

#include "lib/TinyVGM/TinyVGM.h"

#include "lib/ltc6903/ltc6903.h"

// void set_data_offset_abs(const uint32_t o);

/// @brief Returns the absolute offset to VGM data after it has been set
///        by vgm_parse_header()
/// @return Absolute offset to VGM data
uint32_t get_data_offset_abs();

// void set_gd3_offset_abs(const uint32_t o);

/// @brief Returns the absolute offset to GD3 data after it has been set
///        by vgm_parse_header()
/// @return Absolute offset to GD3 data
uint32_t get_gd3_offset_abs();

/// @brief Returns the absolute offset to the loop point after it has been set
///        by vgm_parse_header()
/// @return Absolute offset to loop point
uint32_t get_loop_offset_abs();

typedef struct vgmcb_data_t {
	FIL *fil;
	uint state;
	ltc_handle *ltc_h;
	PIO ym2151_data_pio; // ym2151_write_data PIO inst
	uint ym2151_data_sm; // ym2151_write_data state machine
} vgmcb_data_t;

int vgmcb_header(void *userp, TinyVGMHeaderField field, uint32_t value);
int vgmcb_metadata(void *userp, TinyVGMMetadataType type, uint32_t file_offset, uint32_t len);
int vgmcb_datablock(void *userp, unsigned int type, uint32_t file_offset, uint32_t len);
int vgmcb_command(void *userp, unsigned int cmd, const void *buf, uint32_t len);
int32_t vgmcb_read(void *userp, uint8_t *buf, uint32_t len);
int vgmcb_seek(void *userp, uint32_t pos);
/*
    This library is heavily based on the work 
    TinyVGM by ReimuNotMoe <reimu@sudomaker.com>

    I have modified it to suit my purposes

    This file is part of libvgm.

    Copyright (C) 2022 Anthony Guerrero <anthony@anthony.moe>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif
    
typedef enum {
    VGM_OK = 0,
    VGM_FAIL = -1,
    VGM_EIO = -2,
    VGM_ECANCELED = -3,
    VGM_EINVAL = -4
} VGMReturn;

typedef enum {
	// 0x00
	VGM_HeaderField_Identity = 0,
	VGM_HeaderField_EoF_Offset,
	VGM_HeaderField_Version,
	VGM_HeaderField_SN76489_Clock,

	// 0x10
	VGM_HeaderField_YM2413_Clock,
	VGM_HeaderField_GD3_Offset,
	VGM_HeaderField_Total_Samples,
	VGM_HeaderField_Loop_Offset,

	// 0x20
	VGM_HeaderField_Loop_Samples,
	VGM_HeaderField_Rate,
	VGM_HeaderField_SN_Config,
	VGM_HeaderField_YM2612_Clock,

	// 0x30
	VGM_HeaderField_YM2151_Clock,
	VGM_HeaderField_Data_Offset,
	VGM_HeaderField_SegaPCM_Clock,
	VGM_HeaderField_SPCM_Interface,

	// 0x40
	VGM_HeaderField_RF5C68_Clock,
	VGM_HeaderField_YM2203_Clock,
	VGM_HeaderField_YM2608_Clock,
	VGM_HeaderField_YM2610_Clock,

	// 0x50
	VGM_HeaderField_YM3812_Clock,
	VGM_HeaderField_YM3526_Clock,
	VGM_HeaderField_Y8950_Clock,
	VGM_HeaderField_YMF262_Clock,

	// 0x60
	VGM_HeaderField_YMF278B_Clock,
	VGM_HeaderField_YMF271_Clock,
	VGM_HeaderField_YMZ280B_Clock,
	VGM_HeaderField_RF5C164_Clock,

	// 0x70
	VGM_HeaderField_PWM_Clock,
	VGM_HeaderField_AY8910_Clock,
	VGM_HeaderField_AY_Config,
	VGM_HeaderField_Playback_Config,

	// 0x80
	VGM_HeaderField_GBDMG_Clock,
	VGM_HeaderField_NESAPU_Clock,
	VGM_HeaderField_MultiPCM_Clock,
	VGM_HeaderField_uPD7759_Clock,

	// 0x90
	VGM_HeaderField_OKIM6258_Clock,
	VGM_HeaderField_ArcadeChips_Config,
	VGM_HeaderField_OKIM6295_Clock,
	VGM_HeaderField_K051649_Clock,

	// 0xa0
	VGM_HeaderField_K054539_Clock,
	VGM_HeaderField_HuC6280_Clock,
	VGM_HeaderField_C140_Clock,
	VGM_HeaderField_K053260_Clock,

	// 0xb0
	VGM_HeaderField_Pokey_Clock,
	VGM_HeaderField_QSound_Clock,
	VGM_HeaderField_SCSP_Clock,
	VGM_HeaderField_ExtraHeader_Offset,

	// 0xc0
	VGM_HeaderField_WonderSwan_Clock,
	VGM_HeaderField_VSU_Clock,
	VGM_HeaderField_SAA1099_Clock,
	VGM_HeaderField_ES5503_Clock,

	// 0xd0
	VGM_HeaderField_ES5506_Clock,
	VGM_HeaderField_ES_Config,
	VGM_HeaderField_X1010_Clock,
	VGM_HeaderField_C352_Clock,

	// 0xe0
	VGM_HeaderField_GA20_Clock,

	VGM_HeaderField_MAX

} VGMHeaderField;

typedef enum {
	VGM_MetadataType_Title_EN = 0,
	VGM_MetadataType_Title,
	VGM_MetadataType_Album_EN,
	VGM_MetadataType_Album,
	VGM_MetadataType_SystemName_EN,
	VGM_MetadataType_SystemName,
	VGM_MetadataType_Composer_EN,
	VGM_MetadataType_Composer,
	VGM_MetadataType_ReleaseDate,
	VGM_MetadataType_Converter,
	VGM_MetadataType_Notes,

	VGM_MetadataType_MAX
} VGMMetadataType;

typedef struct vgm_context {
	/*! Callbacks */
	struct {
		/*! Header callback. Params: user pointer, header field, header value */
		int (*header)(void *, VGMHeaderField, uint32_t);

		/*! Metadata callback. Params: user pointer, metadata type, file offset, length */
		int (*metadata)(void *, VGMMetadataType, uint32_t, uint32_t);

		/*! Command callback. Params: user pointer, command, command params, length */
		int (*command)(void *, unsigned int, const void *, uint32_t);

		/*! DataBlock callback. Params: user pointer, data block type, file offset, length */
		int (*data_block)(void *, unsigned int, uint32_t, uint32_t);

		/*! Read callback. Params: user pointer, buffer, length */
		int32_t (*read)(void *, uint8_t *, uint32_t);

		/*! Seek callback. Params: user pointer, file offset */
		int (*seek)(void *, uint32_t);
	} callback;

	/*! User pointer */
	void *userp;
} VGMContext;

/**
 * Get absolute offset of a header item.
 *
 * @param x			Header item enum.
 *
 * @return			Offset
 *
 *
 */
#define vgm_headerfield_offset(x)	((x) * sizeof(uint32_t))

/**
 * Parse the VGM header.
 *
 * @param ctx			VGM context pointer.
 *
 * @return			VGM_OK for success. Errors are reported accordingly.
 *
 *
 */
extern int vgm_parse_header(VGMContext *ctx);

/**
 * Parse the VGM metadata (GD3).
 *
 * @param ctx			VGM context pointer.
 * @param offset_abs		Absolute offset of data in file.
 *
 * @return			VGM_OK for success. Errors are reported accordingly.
 *
 *
 */
extern int vgm_parse_metadata(VGMContext *ctx, uint32_t offset_abs);

/**
 * Parse the VGM commands (incl. data blocks).
 *
 * @param ctx			VGM context pointer.
 * @param offset_abs		Absolute offset of data in file.
 *
 * @return			VGM_OK for success. Errors are reported accordingly.
 *
 *
 */
extern int vgm_parse_commands(VGMContext *ctx, uint32_t offset_abs);
#ifdef __cplusplus
}
#endif
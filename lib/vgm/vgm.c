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

#include "vgm.h"

#ifndef VGM_DEBUG
#define VGM_DEBUG	0
#endif

#if VGM_DEBUG != 1
#define printf
#define fprintf
#endif

// -1: Unused, -2: Data block
static const int8_t vgm_cmd_length_table[256] = {
	//0	1	2	3	4	5	6	7	8	9	A	B	C	D	E	F
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// 00 - 0F
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// 10 - 1F
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// 20 - 2F
	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// 30 - 3F
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	1,	// 40 - 4F
	1,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	// 50 - 5F
	-1,	2,	0,	0,	-1,	-1,	0,	-2,	11,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// 60 - 6F
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 70 - 7F
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 80 - 8F
	4,	4,	5,	10,	1,	4,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// 90 - 9F
	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	// A0 - AF
	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	// B0 - BF
	3,	3,	3,	3,	3,	3,	3,	3,	3,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// C0 - CF
	3,	3,	3,	3,	3,	3,	3,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// D0 - DF
	4,	4,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// E0 - EF
	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	// F0 - FF
};

/// @brief Wrapper function for the context's read function
/// @param ctx VGMContext
/// @param buf Data buffer to read data to
/// @param len Amount of bytes to read
/// @return Number of bytes actually read
static inline int32_t vgm_io_read(VGMContext *ctx, uint8_t *buf, uint32_t len) {
	return ctx->callback.read(ctx->userp, buf, len);
}

static int32_t vgm_io_readall(VGMContext *ctx, uint8_t *buf, uint32_t len) {
	uint32_t bytes_read = 0;

    // Continously try the context's read function until we have read the total
	// number of bytes requested
	while (1) {
        // Read bytes, return number of bytes actually read
		int32_t rc = vgm_io_read(ctx, buf+bytes_read, len-bytes_read);

        // If we read any bytes
		if (rc > 0) {
            // Increment bytes by number of bytes read
			bytes_read += rc;

            // If we have read the total number of bytes needed, return the
            // final number of bytes read
			if (bytes_read == len) {
				return (int32_t)bytes_read;
			}
        // Else, if we didn't read any bytes
		} else if (rc == 0) {
            // Return the number of bytes read, zero.
			return (int32_t)bytes_read;
        // Else, the number of bytes read doesn't match the number requested
		} else {
            // If there were any bytes read, return the number of bytes
			if (bytes_read) {
				return (int32_t)bytes_read;
            // Else, since it's negative, return the user return code
			} else {
				return rc;
			}
		}
	}
}

/// @brief Wrapper function for the context's seek function
/// @param ctx VGMContext
/// @param pos Position to seek the context's pointer to
/// @return VGM_OK, unless errors
static inline int vgm_io_seek(VGMContext *ctx, uint32_t pos) {
	return ctx->callback.seek(ctx->userp, pos);
}

/// @brief 
/// @param ctx VGMContext
/// @return See VGMReturn
/// @details Loops through the header until a stopping point is found, starting
///          with the max allowable header size. Then, if something doesn't match
///          the spec, shortening the loop
int vgm_parse_header(VGMContext *ctx) {
    // Seek the context to zero
	if (vgm_io_seek(ctx, 0) != 0) {
		return VGM_EIO;
	}

    // The value at the pointer
	uint32_t val;

	unsigned int loop_end = VGM_HeaderField_MAX;

    // While the VGM header is still valid
	for (unsigned int i=0; i<loop_end; i++) {

        // Read 32 bits from the context, if 32 bits weren't read, return VGM_EIO
		if (vgm_io_readall(ctx, (uint8_t *) &val, sizeof(uint32_t)) != sizeof(uint32_t)) {
			return VGM_EIO;
		}

        // Print info to stderr, where we are and what the value at the address is
		fprintf(stderr, "vgm_parse_header: offset: 0x%04x, value: 0x%08" PRIx32 " (%" PRId32 ")\n", (unsigned int)(i * sizeof(uint32_t)), val, val);

        // If this is the first iteration of the loop, check the VGM ident.
        // Return INVAL if the VGM header isn't "Vgm "
		if (i == VGM_HeaderField_Identity) {
			if (val != 0x206d6756) {
				return VGM_EINVAL;
			}

            // Let 'em know it's valid
			fprintf(stderr, "vgm_parse_header: valid VGM ident\n");

          // Next, check the VGM file version
		} else if (i == VGM_HeaderField_Version) {
            // Determine where the header parsing needs to end based on version
			if (val < 0x00000151) {
				loop_end = VGM_HeaderField_SegaPCM_Clock;
				if (val < 0x00000150) {
					loop_end = VGM_HeaderField_Data_Offset;
					if (val < 0x00000110) {
						loop_end = VGM_HeaderField_YM2612_Clock;
						if (val < 0x00000101) {
							loop_end = VGM_HeaderField_Rate;
						}
					}
				}
			}

            // If the context has a header parser, every time we loop through the
            // header, call the parser with the offset and the value
			if (ctx->callback.header) {
				int rc = ctx->callback.header(ctx->userp, i, val);

                // If the player doesn't like the VGM file version, return their
                // return value
				if (rc != VGM_OK) {
					return rc;
				}
			}
		} else {
			if (ctx->callback.header) {
				int rc = ctx->callback.header(ctx->userp, i, val);
				
                // Maybe the player doesn't like something else at the header, 
                // return their return value
				if (rc != VGM_OK) {
					return rc;
				}
			}
		}
	}

    // We made it! Return OK
	return VGM_OK;
}

/// @brief Parse the GD3 metadata at the offset provided
/// @param ctx VGMContext
/// @param offset_abs GD3 metadata offset
/// @return See VGMReturn
int vgm_parse_metadata(VGMContext *ctx, uint32_t offset_abs) {
	if (vgm_io_seek(ctx, offset_abs) != 0) {
		return VGM_EIO;
	}

	uint32_t metadata_len = 0;

	uint32_t val;

	// 0: "Gd3 ", 1: version, 2: data len
	for (unsigned int i=0; i<3; i++) {
		
		if (vgm_io_readall(ctx, (uint8_t *) &val, sizeof(uint32_t)) != sizeof(uint32_t)) {
			return VGM_EIO;
		}

		fprintf(stderr, "vgm_parse_metadata: offset: 0x%04x, value: 0x%08" PRIx32 " (%" PRId32 ")\n", (unsigned int)(i * sizeof(uint32_t)), val, val);

		switch (i) {
			case 0:
				if (val != 0x20336447) {
					return VGM_EINVAL;
				}

				fprintf(stderr, "vgm_parse_metadata: valid GD3 ident\n");
				break;

			case 1:
				fprintf(stderr, "vgm_parse_metadata: GD3 version: 0x%08" PRIx32 "\n", val);
				break;

			case 2:
				fprintf(stderr, "vgm_parse_metadata: data len: %" PRIu32 "\n", val);
				metadata_len = val;
				break;
		}

	}

	if (metadata_len) {
		uint32_t cur_pos = offset_abs + 3 * sizeof(uint32_t);
		uint32_t gd3_field_len = 0;
		unsigned int meta_type = VGM_MetadataType_Title_EN;
		uint16_t buf[4];
		uint32_t end_pos = cur_pos + metadata_len;

		while (1) {
			int32_t rc = vgm_io_readall(ctx, (uint8_t *)buf, sizeof(buf));

			if (rc > 0) {
				for (unsigned int i=0; i<(rc/2); i++) {
					if (buf[i]) {
						gd3_field_len += 2;
					} else {
						if (ctx->callback.metadata) {
							int rcc = ctx->callback.metadata(ctx->userp, meta_type, cur_pos, gd3_field_len);

							if (rcc != VGM_OK) {
								return rcc;
							}
						}

						cur_pos += gd3_field_len + 2;
						gd3_field_len = 0;
						meta_type++;
					}
				}

				if (cur_pos >= end_pos) {
					break;
				}
			} else {
				return VGM_EIO;
			}
		}

	}

	return VGM_OK;
}

/// @brief Parse commands until the context doesn't return VGM_OK
/// @param ctx VGMContext
/// @param offset_abs Beginning of VGM command data
/// @return VGMReturn, or user return
int vgm_parse_commands(VGMContext *ctx, uint32_t offset_abs) {
    // Seek to the commands, return if error
	if (vgm_io_seek(ctx, offset_abs) != 0) {
		return VGM_EIO;
	}

    // Absolute position in the VGM file
	uint32_t cur_pos = offset_abs;

    // Command and operand buffer
	uint8_t buf[16];

    // Until something returns !VGM_OK
	while (1) {
        // Buffer for the command
		uint8_t cmd;

        // If reading fails, return IO error
		if (vgm_io_read(ctx, &cmd, 1) != 1) {
			return VGM_EIO;
		}

        // If we return reach the end of sound data, return OK
		if (cmd == 0x66) {
			return VGM_OK;
		}

        // Find the length of the command read
		int8_t cmd_val_len = vgm_cmd_length_table[cmd];

        // If the command is unsupported by the library, return INVALID
		if (cmd_val_len == -1) { // Unused
			fprintf(stderr, "vgm_parse_commands: Unknown command 0x%x\n", cmd);
			return VGM_EINVAL;

        // If it's a data block
		} else if (cmd_val_len == -2) { // Data block
            // Read the data block details to the buffer.
            // If we can't, return IO error
			if (vgm_io_read(ctx, (uint8_t *) &buf, 6) != 6) {
				return VGM_EIO;
			}

            // Get the size of the data block
            // Build the 32 bit length by shifting the 8 bit sections in
			uint32_t pdblen = buf[2];
			pdblen |= ((uint32_t)buf[3] << 8);
			pdblen |= ((uint32_t)buf[4] << 16);
			pdblen |= ((uint32_t)buf[5] << 24);

            // Move the pointer to the beginning of the data
			cur_pos += 1 + 6;

            // If the context has a data block handler, give it the data block
            // type, absolute offset in the file, and the length
			if (ctx->callback.data_block) {
				int rcc = ctx->callback.data_block(ctx->userp, buf[1], cur_pos, pdblen);
                // Maybe the context doesn't like the data block, return its
                // return value
				if (rcc != VGM_OK) {
					return rcc;
				}
			}

            // Move the pointer to the byte after the end of the data block
			cur_pos += pdblen;

            // If we can't seek to the next part of the file, return IO error
			if (vgm_io_seek(ctx, cur_pos) != 0) {
				return VGM_EIO;
			}
        // If not an unsupported command or data block, it's a normal command
		} else {
            // If the command has operands, feed them to the command buffer
			if (cmd_val_len) {
				if (vgm_io_read(ctx, (uint8_t *) &buf, cmd_val_len) != cmd_val_len) {
					return VGM_EIO;
				}
			}

            // Then, call the context's command parser with the command, the
            // operands, and the length of the command
			int rcc = ctx->callback.command(ctx->userp, cmd, buf, cmd_val_len);
            // If the parser doesn't return OK, return its return value
			if (rcc != VGM_OK) {
				return rcc;
			}

            // Advance the pointer by the length of the command plus one (the
            // command itself) and go again
			cur_pos += 1 + cmd_val_len;
		}

	}

}

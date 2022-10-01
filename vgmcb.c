#include "vgmcb.h"

static uint32_t data_offset_abs = 0;
void set_data_offset_abs(const uint32_t o) {data_offset_abs = o;}
uint32_t get_data_offset_abs() {return data_offset_abs;}

static uint32_t loop_offset_abs = 0;
void set_loop_offset_abs(const uint32_t o) {loop_offset_abs = o;}
uint32_t get_loop_offset_abs() {return loop_offset_abs;}

static uint32_t gd3_offset_abs = 0;
void set_gd3_offset_abs(const uint32_t o) {gd3_offset_abs = o;}
uint32_t get_gd3_offset_abs() {return gd3_offset_abs;}

int vgmcb_header(void *userp, TinyVGMHeaderField field, uint32_t value) {
    switch(field) {
        case TinyVGM_HeaderField_Version:
            if(value < 0x00000110) {
                fprintf(stderr, "Can't continue with VGM 1.10 file. No YM2151 chip.");
                return TinyVGM_EINVAL;
            }
            if(value < 0x00000150) {
                set_data_offset_abs(0x40);
                printf("Pre-1.50 version detected, Data Offset is 0x40\n");
            }
            break;
        case TinyVGM_HeaderField_Data_Offset:
            set_data_offset_abs(value + tinyvgm_headerfield_offset(field));
            printf("Data Offset: 0x%08" PRIx32 " (%" PRIu32 ")\n", get_data_offset_abs(), get_data_offset_abs());
            break;
        case TinyVGM_HeaderField_GD3_Offset:
            set_gd3_offset_abs(value + tinyvgm_headerfield_offset(field));
            printf("GD3 Offset: 0x%08" PRIx32 " (%" PRIu32 ")\n", get_gd3_offset_abs(), get_gd3_offset_abs());
            break;
        case TinyVGM_HeaderField_Loop_Offset:
            set_loop_offset_abs(value + tinyvgm_headerfield_offset(field));
            printf("Loop Offset: 0x%08" PRIx32 " (%" PRIu32 ")\n", get_loop_offset_abs(), get_loop_offset_abs());
        case TinyVGM_HeaderField_YM2151_Clock:
            if(!value) {
                fprintf(stderr, "Can't continue. No YM2151 in the VGM\n");
                return TinyVGM_EINVAL;
            }
            sd_card_t *sd_card = sd_get_by_num(0);

            //dma_channel_wait_for_finish_blocking(sd_card->spi->tx_dma);
            //dma_channel_wait_for_finish_blocking(sd_card->spi->rx_dma);
            while(spi_is_busy(sd_card->spi->hw_inst));

            ltc_set_freq(((vgmcb_data_t*)userp)->ltc_h, value);
            //ltc_output_enable(((vgmcb_data_t*)userp)->ltc_h, true);

            printf("Set the YM2151 clock to %d\n", value);
    }
    return TinyVGM_OK;
}

int vgmcb_metadata(void *userp, TinyVGMMetadataType type, uint32_t file_offset, uint32_t len) {
    const char *metadata_strings[] = {
        "Track name: ",
        "Track name (JP): ",
        "Game name: ",
        "Game name (JP): ",
        "System name: ",
        "System name (JP): ",
        "Composer: ",
        "Composer (JP): ",
        "Release date: ",
        "VGM author: ",
        "Notes: "
    };

	// printf("Metadata: Type=%u, FileOffset=%" PRIu32 ", Len=%" PRIu32 ", ", type, file_offset, len);

	FIL *fil = ((vgmcb_data_t*)userp)->fil;
	FSIZE_t cur_pos = f_tell(fil);
	f_lseek(fil, file_offset);

    printf(metadata_strings[type]);

	for (size_t i=0; i<len; i+=2) {
		uint16_t c;
        f_read(fil, &c, 2, NULL);
		if (c > 127) {
			c = '?';
		}
		printf("%lc", c);
	}
	putchar('\n');
    
	f_lseek(fil, cur_pos);

	return TinyVGM_OK;
}

int vgmcb_datablock(void *userp, unsigned int type, uint32_t file_offset, uint32_t len) {
    FIL *fil = ((vgmcb_data_t*)userp)->fil;
    FSIZE_t cur_pos = f_tell(fil);
    f_lseek(fil, file_offset);
    
    /*
    printf("Data block: Type=%u, Offset=%" PRIu32 ", Len=%" PRIu32 "\n", type, file_offset, len);
    for(size_t i = 0; i < len; i++) {
        uint8_t c;
        f_read(fil, &c, 1, NULL);
        printf("%02x ", c);
    }
    puts("");
    */
    
    f_lseek(fil, cur_pos);

    return TinyVGM_OK;
}

int vgmcb_command(void *userp, unsigned int cmd, const void *buf, uint32_t len) {
    PIO pio = ((vgmcb_data_t*)userp)->ym2151_data_pio;
    uint sm = ((vgmcb_data_t*)userp)->ym2151_data_sm;
    uint32_t pio_data;
    
    switch(cmd) {
    case 0x54: // YM2151 data write: 54 aa dd: write value dd to register aa
        uint8_t addr = ((uint8_t*)buf)[0];
        uint8_t data = ((uint8_t*)buf)[1];
        pio_data = 0u | (((uint16_t)addr) << 8) | data;
        pio_sm_put_blocking(pio, sm, pio_data);
        break;
    case 0x61: // Wait n samples: 61 nn nn
        pio_data = 0xffff0000 | *((uint16_t*)buf);
        pio_sm_put_blocking(pio, sm, pio_data);
        break;
    case 0x62: // Wait 735 samples (1/60 second)
        pio_data = 0xffff0000 | 735;
        pio_sm_put_blocking(pio, sm, pio_data);
        break;
    case 0x63: // Wait 882 samples (1/50 second)
        pio_data = 0xffff0000 | 882;
        pio_sm_put_blocking(pio, sm, pio_data);
        break;
    case 0x66: // End of sound data
        return -1;
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7a:
    case 0x7b:
    case 0x7c:
    case 0x7d:
    case 0x7e:
    case 0x7f:
        pio_data = 0xffff0000 | ((cmd & 0x0f) + 1);
        pio_sm_put_blocking(pio, sm, pio_data);
        break;
    default:
        printf("Unhandled command\ncmd: 0x%02x, len: %" PRIu32 ", data: ", cmd, len);
        for(uint32_t i = 0; i < len; i++)
            printf("%02x ", ((uint8_t*)buf)[i]);
        putchar('\n');
        break;
    }
    
    return TinyVGM_OK;
    
}

// should return the number of bytes actually read, 0 for EOF, negative values for error
int32_t vgmcb_read(void *userp, uint8_t *buf, uint32_t len) {
	FIL *fil = ((vgmcb_data_t*)userp)->fil;
    UINT br;
    FRESULT fr = f_read(fil, buf, len, &br);
    
    if(fr == FR_OK) {
        return (int32_t) br; // return # of bytes read
    } else {
        return fr;
    }
}

// should return 0 for success, and negative values for error
int vgmcb_seek(void *userp, uint32_t pos) {
	FIL *fil = ((vgmcb_data_t*)userp)->fil;
    FRESULT fr = f_lseek(fil, pos);
    if(fr == FR_OK) {
        return TinyVGM_OK;
    } else {
        return fr;
    }
}
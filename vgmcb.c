#include "vgmcb.h"

static uint32_t data_offset_abs = 0;
void set_data_offset_abs(const uint32_t o) {data_offset_abs = o;}
uint32_t get_data_offset_abs() {return data_offset_abs;}

static uint32_t gd3_offset_abs = 0;
void set_gd3_offset_abs(const uint32_t o) {gd3_offset_abs = o;}
uint32_t get_gd3_offset_abs() {return gd3_offset_abs;}

int vgmcb_header(void *userp, VGMHeaderField field, uint32_t value) {
    switch(field) {
        case VGM_HeaderField_Version:
            if(value < 0x00000110) {
                fprintf(stderr, "Can't continue with VGM 1.10 file. No YM2151 chip.");
                return VGM_EINVAL;
            }
            if(value < 0x00000150) {
                set_data_offset_abs(0x40);
                printf("Pre-1.50 version detected, Data Offset is 0x40\n");
            }
            break;
        case VGM_HeaderField_Data_Offset:
            set_data_offset_abs(value + vgm_headerfield_offset(field));
            printf("Data Offset: 0x%08" PRIx32 " (%" PRIu32 ")\n", get_data_offset_abs(), get_data_offset_abs());
            break;
        case VGM_HeaderField_GD3_Offset:
            set_gd3_offset_abs(value + vgm_headerfield_offset(field));
            printf("GD3 Offset: 0x%08" PRIx32 " (%" PRIu32 ")\n", get_gd3_offset_abs(), get_gd3_offset_abs());
            break;
        case VGM_HeaderField_YM2151_Clock:
            if(!value) {
                fprintf(stderr, "Can't continue. No YM2151 in the VGM\n");
                return VGM_EINVAL;
            }
            ((vgmcb_data_t*)userp)->ym2151_clock = value;
            printf("Set the YM2151 clock to %d\n", value);
    }
    return VGM_OK;
}

int vgmcb_metadata(void *userp, VGMMetadataType type, uint32_t file_offset, uint32_t len) {
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

	return VGM_OK;
}

int vgmcb_datablock(void *userp, unsigned int type, uint32_t file_offset, uint32_t len) {
    printf("Data block: Type=%u, Offset=%" PRIu32 ", Len=%" PRIu32 "\n", type, file_offset, len);
    
    FIL *fil = ((vgmcb_data_t*)userp)->fil;
    FSIZE_t cur_pos = f_tell(fil);
    f_lseek(fil, file_offset);
    
    for(size_t i = 0; i < len; i++) {
        uint8_t c;
        f_read(fil, &c, 1, NULL);
        printf("%02x ", c);
    }
    puts("");
    
    f_lseek(fil, cur_pos);

    return VGM_OK;
}


int vgmcb_command(void *userp, unsigned int cmd, const void *buf, uint32_t len) {
    static int c = 0;
    if(c < 10) {
        printf("Command: cmd=0x%02x, len=%" PRIu32 ", data:", cmd, len);
    
        for(uint32_t i = 0; i < len; i++) {
            printf("%02x ", ((uint8_t*)buf)[i]); // cast buf to uint8_t pointer, index of i
        }
        puts("");
        c++;
    }
    
    return VGM_OK;
    
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
        return VGM_OK;
    } else {
        return fr;
    }
}
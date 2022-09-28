#include <stdio.h>
#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"

#include "lib/TinyVGM/TinyVGM.h"
#include "vgmcb.h"

FRESULT init_sd_card();
void die(char *msg);

typedef enum {
	VGMPLAYER_PLAY,
	VGMPLAYER_PAUSE,
	VGMPLAYER_STOP
} VGMPLAYER_STATE;


int main(){
	FRESULT fr;
	FATFS fs;
	FIL fil;
	int ret;
	char buf;
	char *filename = "in.vgm";
	
	// Initialize the serial port
	stdio_init_all();
	
	fr = init_sd_card(&fs);
	if(fr != FR_OK) {
		die("Could not initialize the SD card");
	}
	
	fr = f_open(&fil, filename, FA_READ);
	if(fr != FR_OK) {
		printf("f_open returned: %d", fr);
		die("Could not open file");
	}
	
	vgmcb_data_t vgmcb_data = {
		.fil = &fil,
		.state = VGMPLAYER_STOP,
		.ym2151_clock = 0
	};
	
	TinyVGMContext vgm_ctx = {
		.callback = {
			.header = vgmcb_header,
			.metadata = vgmcb_metadata,
			.data_block = vgmcb_datablock,
			
			.command = vgmcb_command,
			.read = vgmcb_read,
			.seek = vgmcb_seek,
		},
		
		.userp = &vgmcb_data
	};
	
	ret = vgm_parse_header(&vgm_ctx);
	
	printf("vgm_parse_header returned %d\n", ret);
	
	if (ret == TinyVGM_OK) {
		if(get_gd3_offset_abs()) {
			ret = vgm_parse_metadata(&vgm_ctx, get_gd3_offset_abs());
			printf("vgm_parse_metadata returned %d\n", ret);
		}
		if (get_data_offset_abs()) {
			ret = vgm_parse_commands(&vgm_ctx, get_data_offset_abs());
			printf("vgm_parse_commands returned %d\n", ret);
		}
	}

	//Loop forever 😢
	f_close(&fil);
	f_unmount("0:");
	while(true) {
		sleep_ms(1000);
	}
}

FRESULT init_sd_card(FATFS *fs) {
	FRESULT fr;
	if(!sd_init_driver()) {
		return fr;
	}
	
	fr = f_mount(fs, "0:", 1);
	if(fr != FR_OK) {
		return fr;
	}
	printf("SD card initialized\n");
	return FR_OK;
}

void die(char *msg) {
	printf("ERROR: %s\n", msg);
	while(true) {
		sleep_ms(1000);
	}
}
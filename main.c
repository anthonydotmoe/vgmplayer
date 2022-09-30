#include <stdio.h>
#include "pico/stdlib.h"
#include "sd_card.h"
#include "ff.h"

#include "lib/TinyVGM/TinyVGM.h"
#include "lib/ltc6903/ltc6903.h"

#include "vgmcb.h"
#include "ym2151.pio.h"

#define YM2151_DATA_BASE 0
#define YM2151_CTRL_BASE 8
#define YM2151_ICL_PIN   14

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
	
	stdio_init_all();
	
	// wait one second
	sleep_ms(1000);
	
	// Set up LTC6903
	printf("Initializing LTC6903\n");
	ltc_handle ltc_h = {
		.cs = 20,
		.oe = 21,
		.spi = spi0
	};
	ltc_initialize(&ltc_h);
	
	// Initialize SD card
	fr = init_sd_card(&fs);
	if(fr != FR_OK) {
		die("Could not initialize the SD card");
	}
	
	// Open file
	fr = f_open(&fil, filename, FA_READ);
	if(fr != FR_OK) {
		printf("f_open returned: %d", fr);
		die("Could not open file");
	}
	

	// Set up user pointer
	vgmcb_data_t vgmcb_data = {
		.fil = &fil,
		.state = VGMPLAYER_STOP,
		.ltc_h = &ltc_h
	};
	
	// Set up TinyVGM
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
	
	// Parse header, get GD3 start, Data offset, determine if file can be played
	ret = tinyvgm_parse_header(&vgm_ctx);
	
	printf("tinyvgm_parse_header returned %d\n", ret);
	
	if (ret == TinyVGM_OK) {
		// Print Metadata
		if(get_gd3_offset_abs()) {
			ret = tinyvgm_parse_metadata(&vgm_ctx, get_gd3_offset_abs());
			printf("tinyvgm_parse_metadata returned %d\n", ret);
		}

		//// Before playing the tune, set up the YM2151
		// Set up ICL gpio
		gpio_init(YM2151_ICL_PIN);
		gpio_set_dir(YM2151_ICL_PIN, GPIO_OUT);
		gpio_put(YM2151_ICL_PIN, 0); // Initial clear with low
		sleep_ms(100);
		gpio_put(YM2151_ICL_PIN, 1); // Set back to high
		// Set up PIO SMs
		PIO pio = pio0;
		// Initialize the timer program
		uint pio_timer_program_offset = pio_add_program(pio, &ym2151_timer_program);
		uint pio_timer_program_sm = pio_claim_unused_sm(pio, true);
		init_ym2151_timer_program(pio, pio_timer_program_sm, pio_timer_program_offset);
		// Initialize the data program
		uint pio_data_program_offset = pio_add_program(pio, &ym2151_write_data_program);
		uint pio_data_program_sm = pio_claim_unused_sm(pio, true);
		init_ym2151_write_data_program(pio, pio_data_program_sm, pio_data_program_offset, 0, 8);
		
		// Share this data with the callback
		vgmcb_data.ym2151_data_pio = pio;
		vgmcb_data.ym2151_data_sm = pio_data_program_sm;
		
		// Play the tune
		if (get_data_offset_abs()) {
			ret = tinyvgm_parse_commands(&vgm_ctx, get_data_offset_abs());
			printf("tinyvgm_parse_commands returned %d\n", ret);
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
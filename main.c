#include <stdio.h>
#include <stdarg.h>
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
#define YM2151_ICL_DELAY 100

FRESULT init_sd_card();
void __attribute((noreturn)) die(const char *fmt, ...);

typedef enum {
	VGMPLAYER_PLAY,
	VGMPLAYER_PAUSE,
	VGMPLAYER_STOP
} VGMPLAYER_STATE;


int main(){
	FRESULT fr;
	FATFS fs;
	DIR root_dir;
	FILINFO filinfo;
	FIL fil;

	int ret;
	char buf;
	
	stdio_init_all();
	
	sleep_ms(500); // Wait for computer to catch up

	// Set up _ICL pin and silence the chips
	gpio_init(YM2151_ICL_PIN);
	gpio_set_dir(YM2151_ICL_PIN, GPIO_OUT);
	gpio_put(YM2151_ICL_PIN, 1);
	sleep_ms(YM2151_ICL_DELAY);
	gpio_put(YM2151_ICL_PIN, 0);
	sleep_ms(YM2151_ICL_DELAY);
	gpio_put(YM2151_ICL_PIN, 1);
	
	// Set up LTC6903
	printf("Initializing LTC6903\n");
	ltc_handle ltc_h = {
		.cs = 20,
		.oe = 21,
		.spi = spi0
	};
	ltc_initialize(&ltc_h);

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

	// Initialize SD card
	fr = init_sd_card(&fs);
	if(fr != FR_OK) {
		die("Could not initialize the SD card");
	}

	fr = f_findfirst(&root_dir, &filinfo, "/", "*.vgm");
	if(fr != FR_OK) {
		die("searching for *.vgm, FatFS returned %d", fr);
	}

	while (*filinfo.fname) {
		// Open file
		fr = f_open(&fil, filinfo.fname, FA_READ);
		if (fr != FR_OK)
		{
			die("Could not open \"%s\", f_open returned %d", filinfo.fname, fr);
		}
		printf("Opened file: %s\n", filinfo.fname);

		// Parse header, get GD3 start, Data offset, determine if file can be played
		ret = tinyvgm_parse_header(&vgm_ctx);

		printf("tinyvgm_parse_header returned %d\n", ret);

		if (ret == TinyVGM_OK)
		{
			// Print Metadata
			if (get_gd3_offset_abs())
			{
				ret = tinyvgm_parse_metadata(&vgm_ctx, get_gd3_offset_abs());
				printf("tinyvgm_parse_metadata returned %d\n", ret);
			}
			
			// Play the tune
			if (get_data_offset_abs())
			{
				uint loop_count = 0;
				pio_sm_set_enabled(pio, pio_timer_program_sm, false);
				pio_sm_clear_fifos(pio, pio_data_program_sm);
				pio_sm_restart(pio, pio_data_program_sm);
				pio_sm_set_enabled(pio, pio_timer_program_sm, true);

				gpio_put(YM2151_ICL_PIN, 0); // Initial clear with low
				sleep_ms(YM2151_ICL_DELAY);
				gpio_put(YM2151_ICL_PIN, 1); // Set back to high
				sleep_ms(YM2151_ICL_DELAY);
				ret = tinyvgm_parse_commands(&vgm_ctx, get_data_offset_abs());
				/*
				while (!ret && loop_count < 1) {
					ret = tinyvgm_parse_commands(&vgm_ctx, get_loop_offset_abs());
					loop_count++;
				}
				*/
				printf("tinyvgm_parse_commands returned %d\n", ret);
			}
			printf("Playback finished\n");
		}
		sleep_ms(100);
		f_close(&fil);
		f_findnext(&root_dir, &filinfo);
	}

	f_unmount("0:");
	die("Out of vgm files");
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

void __attribute((noreturn)) die(const char *fmt, ...) {
	va_list args;

	printf("ERROR: ");
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
	while(true) {
		sleep_ms(1000);
	}
}
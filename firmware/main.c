#include <hardware/pio.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "pico/stdio.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/sync.h"

#include "sd_card.h"
#include "ff.h"

#include "lib/TinyVGM/TinyVGM.h"
#include "lib/ltc6903/ltc6903.h"

#include "pbcmd.h"
#include "vgmcb.h"
#include "ym2151.pio.h"

#define YM2151_DATA_BASE 0
#define YM2151_CTRL_BASE 8
#define YM2151_ICL_PIN   14
#define YM2151_ICL_DELAY 100

static FRESULT init_sd_card(FATFS *fs);
static void __attribute((noreturn)) die(const char *fmt, ...);
static void core1_main(void);

typedef enum {
	VGMPLAYER_PLAY,
	VGMPLAYER_PAUSE,
	VGMPLAYER_STOP
} VGMPLAYER_STATE;

typedef struct _Core1State {
	PIO ym2151_data_pio;
	uint ym2151_data_sm;
} Core1State;

static Core1State core1_state;

static void ym2151_reset()
{
	gpio_put(YM2151_ICL_PIN, 0);
	sleep_ms(YM2151_ICL_DELAY);
	gpio_put(YM2151_ICL_PIN, 1);
}

int main(){
	FRESULT fr;
	FATFS fs;
	DIR root_dir;
	FILINFO filinfo;
	FIL fil;

	int ret;
	char buf;

	// Workaround for sleep_ms hanging the debug session
	timer_hw->dbgpause = 0;
	
	stdio_init_all();

	// Initialize ICL pin
	gpio_init(YM2151_ICL_PIN);
	gpio_set_dir(YM2151_ICL_PIN, GPIO_OUT);
	gpio_put(YM2151_ICL_PIN, 1);

	sleep_ms(500); // Wait for computer to catch up

	// Initialize the shared queue
	pbcmd_init();
	
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
	uint pio_data_program_offset = pio_add_program(pio, &ym2151_write_data_program);
	uint pio_data_program_sm = pio_claim_unused_sm(pio, true);
	init_ym2151_write_data_program(pio, pio_data_program_sm, pio_data_program_offset, 0, 8);

	// Share this data with the callback
	core1_state.ym2151_data_pio = pio;
	core1_state.ym2151_data_sm = pio_data_program_sm;

	// Start up the playback core
	multicore_launch_core1(core1_main);

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

				ret = tinyvgm_parse_commands(&vgm_ctx, get_data_offset_abs());
				
				// Loop it forever
				while (!ret) {
					ret = tinyvgm_parse_commands(&vgm_ctx, get_loop_offset_abs());
				}
				printf("tinyvgm_parse_commands returned %d\n", ret);
			}
			//TODO: We just die here because playback would likely stop immediately
			die("Can't handle Core 1 running on it's own...");
			printf("Playback finished\n");
		}
		sleep_ms(100);
		f_close(&fil);
		f_findnext(&root_dir, &filinfo);
	}

	f_unmount("0:");
	die("Out of vgm files");
}

static FRESULT init_sd_card(FATFS *fs) {
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

static void __attribute((noreturn)) die(const char *fmt, ...) {
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

static volatile int32_t wait_accum = 0;

static bool vgm_sample_timer_cb(repeating_timer_t *rt)
{
	wait_accum--;
	return true;	// Keep repeating
}

static void emit_ym_write(uint8_t addr, uint8_t data);

static void core1_main()
{
	// This function is started after Core 0 is done touching Core1State
	
	// TODO:
	// This is not exactly 44.1kHz, testing only
	static repeating_timer_t sample_timer;
	add_repeating_timer_us(
		-23,
		vgm_sample_timer_cb,
		NULL,
		&sample_timer
	);

	PlaybackCmd cmd;

	while (1)
	{
		pbcmd_pull(&cmd);
		switch (cmd.ty) {
			case CMD_Reset:
				pio_sm_clear_fifos(
					core1_state.ym2151_data_pio,
					core1_state.ym2151_data_sm);
				pio_sm_restart(
					core1_state.ym2151_data_pio,
					core1_state.ym2151_data_sm);
				ym2151_reset();
				break;
			case CMD_Wait:
				wait_accum += cmd.cmd_wait.wait;
				break;
			case CMD_Write:
				while (wait_accum > 0) {
					// Wait for time to pass
				}

				emit_ym_write(cmd.cmd_write.addr, cmd.cmd_write.data);
				break;

			default:
				break;
		}
	}

}

static void emit_ym_write( uint8_t addr, uint8_t data)
{
	uint32_t pio_data = (((uint16_t)addr) << 8) | data;
	PIO pio = core1_state.ym2151_data_pio;
	pio_sm_put_blocking(
		core1_state.ym2151_data_pio,
		core1_state.ym2151_data_sm,
		pio_data);
}

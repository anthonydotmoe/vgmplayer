#pragma once

#include <stdint.h>

#define RING_SIZE 1024

typedef enum _CmdType {
    CMD_Reset,
    CMD_Write,
    CMD_Wait,
} CmdType;

typedef struct _PlaybackCmd {
    CmdType ty;
    union {
        struct {
            uint32_t ym_clock;
        } cmd_reset;

        struct {
            uint8_t addr;
            uint8_t data;
        } cmd_write;

        struct {
            int wait;
        } cmd_wait;
    };
} PlaybackCmd;

void pbcmd_init(void);

void pbcmd_push(PlaybackCmd *cmd);

void pbcmd_pull(PlaybackCmd *cmd);

#include "pbcmd.h"

#include "pico/util/queue.h"

// Static command ring buffer
static queue_t pbcmd_queue;
static volatile uint32_t wr_pos;
static volatile uint32_t rd_pos;

void pbcmd_init()
{
    queue_init(&pbcmd_queue, sizeof(PlaybackCmd), RING_SIZE);
}

void pbcmd_push(PlaybackCmd *cmd)
{
    queue_add_blocking(&pbcmd_queue, cmd);
}

void pbcmd_pull(PlaybackCmd *cmd)
{
    queue_remove_blocking(&pbcmd_queue, cmd);
}

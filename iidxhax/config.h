#ifndef __IIDXHAX_CONFIG__
#define __IIDXHAX_CONFIG__

#include <stdbool.h>
#include <stdint.h>

struct iidxhax_config {
    // system
    bool omnimix;
    bool bms;
    bool skip_camera_error;
    bool lock_fps_enabled;
    float lock_fps_value;
    bool monitor_adjust_enabled;
    int32_t monitor_adjust_value;
    bool debug_mode;
    bool patch_sse42;

    // mode select
    bool premium_free;
    bool expert_mode;

    // song select
    bool timer_freeze;
    bool premium_timer_freeze;
    bool cursor_lock;
    bool unlock_all;
    bool remove_song_select_color_banners;
    bool all_video_preview;
    bool skip_decide_screen;

    // song
    bool cs_song_delay;
    bool dark_mode;
    bool hide_measure_lines;
    bool quick_retry;
    bool all_notes_preview;
    bool show_fs_total;

    // ticker_output
    bool top_left;
    bool freeplay_ticker;
    bool notavailable_ticker;
    bool hide_all_text;
};

#endif
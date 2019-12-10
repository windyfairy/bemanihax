#ifndef __DDRHAX_CONFIG__
#define __DDRHAX_CONFIG__

#include <stdbool.h>
#include <stdint.h>

struct ddrhax_config {
    bool mute_announcer;
    bool freeze_timer;
    bool skip_tutorial;
    bool fast_slow;
    bool judge_bg;
    bool darkest_bg;
    bool arrow_color;
    bool ddr_selection;
    bool unlock_all;
    bool pfree;
};

#endif
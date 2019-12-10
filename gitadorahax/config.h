#ifndef __GITADORAHAX_CONFIG__
#define __GITADORAHAX_CONFIG__

#include <stdbool.h>

struct gitadorahax_config {
    bool omnimix;
    bool timer_freeze;
    bool stage_freeze;
    bool all_music;
    bool scorecard_hook;
    bool display_timing_stats;
    int machine_type;
};

#endif
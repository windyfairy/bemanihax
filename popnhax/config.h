#ifndef __POPNHAX_CONFIG__
#define __POPNHAX_CONFIG__

#include <stdbool.h>

struct popnhax_config {
    bool unset_volume;
    bool event_mode;
    bool remove_timer;
    bool freeze_timer;
    bool unlock_all;
    bool skip_tutorials;
};

#endif
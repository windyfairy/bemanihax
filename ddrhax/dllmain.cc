// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "util/fuzzy_search.h"

#include "minhook/hde32.h"
#include "minhook/include/MinHook.h"

#include "ddrhax/config.h"
#include "util/patch.h"
#include "util/xmlprop.hpp"

struct ddrhax_config config = {};

PSMAP_BEGIN(config_psmap, static)
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, mute_announcer,
                 "/ddrhax/mute_announcer")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, freeze_timer,
                 "/ddrhax/freeze_timer")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, skip_tutorial,
                 "/ddrhax/skip_tutorial")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, fast_slow, "/ddrhax/fast_slow")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, judge_bg, "/ddrhax/judge_bg")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, darkest_bg, "/ddrhax/darkest_bg")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, arrow_color, "/ddrhax/arrow_color")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, ddr_selection,
                 "/ddrhax/ddr_selection")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, unlock_all, "/ddrhax/unlock_all")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct ddrhax_config, pfree, "/ddrhax/pfree")
PSMAP_END

static bool patch_mute_crowd() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xC6\x40\x85\xC0\x0F\x84", 6)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xC6\x40\x85\xC0\x90\xE9", 6);
    }

    printf("ddrhax: announcer muted everywhere\n");

    return true;
}

static bool patch_mute_announcer() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "voice.xwb", 9)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"boice.xwb", 9);
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "voice_n.xwb", 11)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"boice_n.xwb", 11);
    }

    printf("ddrhax: crowd muted during stage\n");

    return true;
}

static bool patch_freeze_timer() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x74\x40\xBF", 3)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xEB", 1);
    }

    printf("ddrhax: timer frozen\n");

    return true;
}

static bool patch_skip_tutorial() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x47\x70\x05\x00\x00\x00\x33", 7)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x10\x8B\x08\x83\x39\x01\x74", 7)

        int64_t pattern_offset = find_block(data, 0x140, &task, first_loc);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x10\x8B\x08\x83\x39\x01\xEB", 7);
    }

    printf("ddrhax: tutorial skipped\n");

    return true;
}

static bool patch_fast_slow() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x8B\x41\x44\xC3", 4)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x31\xC0\x40", 3);
    }

    printf("ddrhax: fast/slow indicators forced\n");

    return true;
}

static bool patch_judge_bg() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x8B\x41\x40\xC3", 4)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x31\xC0", 2);
    }

    printf("ddrhax: judgment background forced\n");

    return true;
}

// doesnt work with 2018042300
static bool patch_darkest_bg() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x75\x03\x33\xC0\xC3\x8B\x41\x34\xC3", 9)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x33\xC0\xB0\x03", 4);
    }

    printf("ddrhax: darkest background forced\n");

    return true;
}

static bool patch_arrow_color() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x10\x80\x7D\xFF\x00\x75\x3A", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x10\x80\x7D\xFF\x00\xEB\x3A", 7);
    }

    printf("ddrhax: eA exclusive options (arrow color) forced\n");

    return true;
}

static bool patch_ddr_selection() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xBE\x1E\x00\x00\x00\xE8", 6)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xBE\x1E\x00\x00\x00\xB8\x01\x00\x00", 9);
    }

    printf("ddrhax: DDR selection unlocked\n");

    return true;
}

// only works with 2019042200 (and above maybe), fix later
static bool patch_unlock_all() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x75\x08\x5F\x32\xC0", 5)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x90\x90\x5F\xB0\x01", 5);
    }

    printf("ddrhax: all songs unlocked\n");

    return true;
}

static bool patch_pfree() {
    DWORD dllSize = 0;
    char *data = getDllData("gamemdx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x10\xB9\x01\x00\x00\x00\x89\x0D", 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x10\xB9\x00\x00\x00\x00\x89\x0D", 8);
    }

    printf("ddrhax: fake pfree (first stage forced)\n");

    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        if (MH_Initialize() != MH_OK) {
            printf("Failed to initialize minhook\n");
            exit(1);
            return TRUE;
        }

        LoadLibrary("gamemdx.dll");

        _load_config("ddrhax.xml", &config, config_psmap);

        if (config.mute_announcer) {
            patch_mute_crowd();
            patch_mute_announcer();
        }

        if (config.freeze_timer) {
            patch_freeze_timer();
        }

        if (config.skip_tutorial) {
            patch_skip_tutorial();
        }

        if (config.fast_slow) {
            patch_fast_slow();
        }

        if (config.judge_bg) {
            patch_judge_bg();
        }

        if (config.darkest_bg) {
            patch_darkest_bg();
        }

        if (config.arrow_color) {
            patch_arrow_color();
        }

        if (config.ddr_selection) {
            patch_ddr_selection();
        }

        if (config.unlock_all) {
            patch_unlock_all();
        }

        if (config.pfree) {
            patch_pfree();
        }

        MH_EnableHook(MH_ALL_HOOKS);

        break;
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

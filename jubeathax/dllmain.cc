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

#include "jubeathax/config.h"
#include "util/patch.h"
#include "util/xmlprop.hpp"

struct jubeathax_config config = {};

PSMAP_BEGIN(config_psmap, static)
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct jubeathax_config, disable_matching,
                 "/jubeathax/disable_matching")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct jubeathax_config, freeze_menu_timer,
                 "/jubeathax/freeze_menu_timer")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct jubeathax_config, freeze_result_timer,
                 "/jubeathax/freeze_result_timer")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct jubeathax_config, skip_tutorial,
                 "/jubeathax/skip_tutorial")
PSMAP_END

static bool patch_disable_matching() {
    DWORD dllSize = 0;
    char *data = getDllData("jubeat.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x00\x8B\xD7\x33\xC9\xE8", 6)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x84", 1)

        int64_t pattern_offset = find_block(data, 0x15, &task, first_loc);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x85", 1);
    }

    printf("jubeathax: matching disabled\n");

    return true;
}

static bool patch_menu_timer() {
    DWORD dllSize = 0;
    char *data = getDllData("jubeat.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\x01\x00\x84\xC0\x75", 5)
        FUZZY_WILDCARD(task, 1, 1)
        FUZZY_CODE(task, 2, "\x38\x05", 2)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x01\x00\x84\xC0\xEB", 5);

        printf("jubeathax: menu timer frozen\n");
    }

    return true;
}

static bool patch_result_timer() {
    DWORD dllSize = 0;
    char *data = getDllData("jubeat.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x8B\xF0\x32\xDB\x6A\x03\x6A\x02", 8)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x00\x75", 2)

        int64_t pattern_offset = find_block(data, 0x60, &task, first_loc);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x00\xEB", 2);
    }

    printf("jubeathax: result timer frozen at 0 seconds\n");

    return true;
}

static bool patch_skip_tutorial() {
    DWORD dllSize = 0;
    char *data = getDllData("jubeat.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x75\x75\x56\x68\x00\x00\x60\x23", 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\xEB\x75\x56\x68\x00\x00\x60\x23", 8);
            printf("jubeathax: tutorial skipped\n");
            return true;
        }
        else {
            fuzzy_search_task task1;

            FUZZY_START(task1, 1)
            FUZZY_CODE(task1, 0, "\x75\x21\x38\x05\x20\x7A\xDB\x1B", 8)

            int64_t pattern_offset1 = find_block(data, dllSize, &task1, 0);
            if (pattern_offset1 != -1) {
                uint64_t patch_addr = (int64_t)data + pattern_offset1;
                patch_memory(patch_addr, (char *)"\xEB\x21\x38\x05\x20\x7A\xDB\x1B", 8);
                printf("jubeathax: tutorial skipped\n");
                return true;
            }
        }
    }
    return false;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        if (MH_Initialize() != MH_OK) {
            printf("Failed to initialize minhook\n");
            exit(1);
            return TRUE;
        }

        _load_config("jubeathax.xml", &config, config_psmap);

        if (config.disable_matching) {
            patch_disable_matching();
        }

        if (config.freeze_menu_timer) {
            patch_menu_timer();
        }

        if (config.freeze_result_timer) {
            patch_result_timer();
        }

        if (config.skip_tutorial) {
            patch_skip_tutorial();
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

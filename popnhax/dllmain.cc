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

#include "popnhax/config.h"
#include "util/patch.h"
#include "util/xmlprop.hpp"

struct popnhax_config config = {};

PSMAP_BEGIN(config_psmap, static)
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct popnhax_config, unset_volume,
                 "/popnhax/unset_volume")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct popnhax_config, event_mode, "/popnhax/event_mode")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct popnhax_config, remove_timer,
                 "/popnhax/remove_timer")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct popnhax_config, freeze_timer,
                 "/popnhax/freeze_timer")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct popnhax_config, unlock_all, "/popnhax/unlock_all")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct popnhax_config, skip_tutorials,
                 "/popnhax/skip_tutorials")
PSMAP_END

static bool patch_unset_volume() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x04\x00\x81\xC4\x00\x01\x00\x00\xC3\xCC", 10)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x83", 1)

        int64_t pattern_offset = find_block(data, 0x10, &task, first_loc);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xC3", 1);
    }

    printf("popnhax: windows volume untouched\n");

    return true;
}

static bool patch_event_mode() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0,
                   "\x8B\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC"
                   "\xCC\xCC\xC7",
                   17)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x31\xC0\x40\xC3", 4);
    }

    printf("popnhax: event mode forced\n");

    return true;
}

static bool patch_remove_timer() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x8B\xAC\x24\x68\x01", 5)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x0F", 1)

        int64_t pattern_offset = find_block(data, 0x15, &task, first_loc);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x90\xE9", 2);
    }

    printf("popnhax: timer removed\n");

    return true;
}

static bool patch_freeze_timer() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xC7\x45\x38\x09\x00\x00\x00", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x90\x90\x90\x90\x90\x90\x90", 7);
    }

    printf("popnhax: timer frozen at 10 seconds remaining\n");

    return true;
}

static bool patch_hidden_charts_3() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    bool patched = false;
    while (1) {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x80\x00\x00\x03", 4)

        int64_t pattern_offset = find_block(data, 0x50000, &task, 0x2A0000);
        if (pattern_offset == -1) {
            break;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;

        //        printf("popnhax: found hidden chart (03) at %llx\n", patch_addr);
        patch_memory(patch_addr, (char *)"\x00\x00\x00\x03", 4);
        patched = true;
    }

    if (patched) {
        printf("popnhax: found and unlocked hidden charts (03)\n");
    }

    return patched;
}

static bool patch_hidden_charts_7() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    bool patched = false;
    while (1) {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x80\x00\x00\x07", 4)

        int64_t pattern_offset = find_block(data, 0x50000, &task, 0x2A0000);
        if (pattern_offset == -1) {
            break;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;

        //        printf("popnhax: found hidden chart (07) at %llx\n", patch_addr);
        patch_memory(patch_addr, (char *)"\x00\x00\x00\x07", 4);
        patched = true;
    }

    if (patched) {
        printf("popnhax: found and unlocked hidden charts (07)\n");
    }

    return patched;
}

static bool patch_unlock_classic8() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x10\xF1\x01\xF2\x01", 5)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x00\x05", 2)

        int64_t pattern_offset = find_block(data, 0x10, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x00\x07", 2);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x26\x01", 2)

        int64_t pattern_offset = find_block(data, 0x20, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x26\x31", 2);

            printf("popnhax: classic 8 EX (49) unlocked\n");
        }
    }

    return true;
}

static bool patch_unlock_all() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xC3\x69\xC0\x34\x04", 5)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xD0\x74", 2)

        int64_t pattern_offset = find_block(data, 0x60, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\xD0\xEB", 2);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x83\xFE\x0A\x75", 4)

        int64_t pattern_offset = find_block(data, 0x80, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x83\xFE\x0A\x90\x90", 5);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x00\x08\x74", 3)

        int64_t pattern_offset = find_block(data, 0x90, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x00\x08\x90\x90", 4);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xFF\xFF\xFF\x84\xC0\x74", 6)

        int64_t pattern_offset = find_block(data, 0x100, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\xFF\xFF\xFF\xB0\x01\x74", 6);

            printf("popnhax: all songs and characters unlocked\n");
        }
    }

    return true;
}

static bool patch_skip_tutorials() {
    DWORD dllSize = 0;
    char *data = getDllData("popn22.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xFD\xFF\x5E\xC2\x04\x00\xE8", 7)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x74", 1)

        int64_t pattern_offset = find_block(data, 0x10, &task, first_loc);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xEB", 1);
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x66\x85\xC0\x75\x5E\x6A", 6)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x66\x85\xC0\xEB\x5E\x6A", 6);
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x00\x5F\x5E\x66\x83\xF8\x01\x75", 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x00\x5F\x5E\x66\x83\xF8\x01\xEB", 8);
    }

    printf("popnhax: menu and long note tutorials skipped\n");

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

        _load_config("popnhax.xml", &config, config_psmap);

        if (config.unset_volume) {
            patch_unset_volume();
        }

        if (config.event_mode) {
            patch_event_mode();
        }

        if (config.remove_timer) {
            patch_remove_timer();
        }

        if (config.freeze_timer) {
            patch_freeze_timer();
        }

        if (config.unlock_all) {
            // patch_hidden_charts_3();
            // patch_hidden_charts_7();
            patch_unlock_classic8();
            patch_unlock_all();
        }

        if (config.skip_tutorials) {
            patch_skip_tutorials();
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

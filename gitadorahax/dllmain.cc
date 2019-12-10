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

#include "util/patch.h"
#include "util/xmlprop.hpp"

#include "gitadorahax/config.h"
#include "gitadorahax/gitadorascorehook.h"

struct gitadorahax_config config = {};

PSMAP_BEGIN(config_psmap, static)
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_S32, struct gitadorahax_config, machine_type,
                 "/gitadorahax/machine_type")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct gitadorahax_config, omnimix,
                 "/gitadorahax/omnimix")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct gitadorahax_config, timer_freeze,
                 "/gitadorahax/timer_freeze")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct gitadorahax_config, stage_freeze,
                 "/gitadorahax/stage_freeze")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct gitadorahax_config, all_music,
                 "/gitadorahax/all_music")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct gitadorahax_config, scorecard_hook,
                 "/gitadorahax/scorecard_hook")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct gitadorahax_config, display_timing_stats,
                 "/gitadorahax/display_timing_stats")
PSMAP_END

static void (*real_ea3_boot)(struct property_node *conf);

void (*real_omnimix_x_internal)();
void patch_omnimix_x_internal() {
    // avs hasn't changed for Gitadora since the beginning of time so this should be safe
    __asm("mov rsi, [rbx+0x38]\n");
    __asm("mov byte ptr [rsi+0x08], 'X'\n");

    real_omnimix_x_internal();
}

static bool patch_omnimix_x() {
    DWORD dllSize = 0;
    char *data = getDllData("libavs-win64-ea3.dll", &dllSize);

    // Patch *:*:A that is sent through network requests
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x48\x89\xCE\x48\x89\xD7\x4C\x89\xC2\x45\x89\xC8", 12)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        MH_CreateHook(data + pattern_offset, (void *)&patch_omnimix_x_internal,
                      (void **)&real_omnimix_x_internal);

        MH_EnableHook(MH_ALL_HOOKS);

        printf("gitadorahax: hooked omnimix x\n");
    }

    return true;
}

char *(*real_sys_code_get_soft_id_code)();
char *sys_code_get_soft_id_code() {
    // Patch *:*:A that is shown in-game on the screen (only visual)
    char *soft_id_code = real_sys_code_get_soft_id_code();
    soft_id_code[8] = 'X';
    return soft_id_code;
}

static bool patch_omnimix() {
    DWORD dllSize = 0;
    char *data = getDllData("game.dll", &dllSize);

    // Hook mdb_*.xml
    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "mdb_", 4)
        FUZZY_WILDCARD(task, 1, 2)
        FUZZY_CODE(task, 2, ".xml", 4)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"odb_", 4);

        printf("gitadorahax: hooked mdb_*.xml\n");
    }

    // Hook notes_info.xml
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "notes_info.xml", 14)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"omni_notes.xml", 14);

            printf("gitadorahax: hooked notes_info.xml\n");
        }
    }

    // Hook phrase_address_list.xml
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "phrase_address_list.xml", 23)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"omni_phrase_address.xml", 23);

            printf("gitadorahax: hooked phrase_address_list.xml\n");
        }
    }

    return true;
}

static bool patch_timer_freeze() {
    DWORD dllSize = 0;
    char *data = getDllData("game.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x0f\x85\xaa\x01\x00\x00\x8b", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xe9\xab\x01\x00\x00\x90", 6);

        printf("gitadorahax: timer frozen\n");
    }

    return true;
}

static bool patch_stage_freeze() {
    DWORD dllSize = 0;
    char *data = getDllData("game.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x84\xC0", 2)
        FUZZY_CODE(task, 1, "\x0f\x85\xfb\x01\x00\x00", 6)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr, (char *)"\xe9\xfc\x01\x00\x00\x90", 6);

        printf("gitadorahax: stage counter frozen\n");
    }

    return true;
}

static bool patch_all_music() {
    DWORD dllSize = 0;
    char *data = getDllData("game.dll", &dllSize);

    int64_t xg_secret_addr = -1;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "xg_secret\0", 10)

        xg_secret_addr = find_block(data, dllSize, &task, 0);
        if (xg_secret_addr == -1) {
            return false;
        }
    }

    if (xg_secret_addr != -1) {
        fuzzy_search_task task;

        uint64_t offset = (int64_t)data + xg_secret_addr;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x02\x00\x00\x00", 4)
        FUZZY_CODE(task, 1, (char *)&offset, 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset - 2;
        patch_memory(patch_addr, (char *)"\x4d\x01", 2);
    }

    int64_t secret_addr = -1;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "secret\0", 7)

        secret_addr = find_block(data, dllSize, &task, xg_secret_addr + 10);
        if (secret_addr == -1) {
            return false;
        }
    }

    if (secret_addr != -1) {
        fuzzy_search_task task;

        uint64_t offset = (int64_t)data + secret_addr;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x02\x00\x00\x00", 4)
        FUZZY_CODE(task, 1, (char *)&offset, 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset - 2;
        patch_memory(patch_addr, (char *)"\x4d\x01", 2);
    }

    int64_t jump_offset = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x85\xC9", 2)
        FUZZY_CODE(task, 1, "\x75\x16\x38\x8B", 4)

        jump_offset = find_block(data, dllSize, &task, 0);
        if (jump_offset == -1) {
            return false;
        }

        jump_offset += task.blocks[0].data[0].length;
    }

    int64_t ret_offset = 0;
    if (jump_offset != -1) {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x33\xc0\x48\x8B", 4)

        ret_offset = find_block(data, dllSize, &task, jump_offset);
        if (ret_offset == -1) {
            return false;
        }
    }

    if (jump_offset != -1 && ret_offset != -1) {
        // Patch jump using new offset
        uint64_t patch_addr = (int64_t)data + jump_offset;
        int32_t new_offset = ret_offset - (jump_offset + 2); // 2 = size of original jnz command
        patch_memory(patch_addr, (char *)"\xeb", 1);
        patch_memory(patch_addr + 1, (char *)&new_offset, 1);
    }

    // Long version songs
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x75\x03\x33\xC0\xC3\x8B", 6)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    printf("gitadorahax: all music flag set\n");

    return true;
}

static int64_t (*real_game_initialize)(HWND);
static bool patch_machine_type(HWND param1) {
    typedef int(__stdcall * CSysSettingSetParam)(const char *key, int32_t value);
    CSysSettingSetParam sys_setting_set_param = reinterpret_cast<CSysSettingSetParam>(
        ::GetProcAddress(LoadLibraryA("libsystem.dll"), "sys_setting_set_param"));
    sys_setting_set_param("VER_MACHINE", config.machine_type << 12);
    printf("Setting VER_MACHINE to %04x\n", config.machine_type << 12);
    real_game_initialize(param1);
    return true;
}

void ea3_boot(struct property_node *conf) {
    if (config.omnimix) {
        patch_omnimix_x();

        MH_CreateHookApi(L"libsystem.dll", "sys_code_get_soft_id_code",
                         (void *)&sys_code_get_soft_id_code,
                         (void **)&real_sys_code_get_soft_id_code);
        MH_EnableHook(MH_ALL_HOOKS);
    }

    real_ea3_boot(conf);

    if (config.omnimix) {
        patch_omnimix();
    }

    if (config.timer_freeze) {
        patch_timer_freeze();
    }

    if (config.stage_freeze) {
        patch_stage_freeze();
    }

    if (config.all_music) {
        patch_all_music();
    }

    if (config.machine_type < 0 || config.machine_type > 3) {
        config.machine_type = 0;
    }

    if (config.scorecard_hook || config.display_timing_stats) {
        scorehook_init(config.scorecard_hook, config.display_timing_stats);
    }

    MH_CreateHookApi(L"game.dll", "?game_initialize@@YAHPEAUHWND__@@@Z",
                     (void *)&patch_machine_type, (void **)&real_game_initialize);
    MH_EnableHook(MH_ALL_HOOKS);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        if (MH_Initialize() != MH_OK) {
            printf("Failed to initialize minhook\n");
            exit(1);
            return TRUE;
        }

        _load_config("gitadorahax.xml", &config, config_psmap);

        // Hook ea3_boot
        MH_CreateHookApi(L"libavs-win64-ea3.dll", "XE592acd00008c", (void *)&ea3_boot,
                         (void **)&real_ea3_boot);

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

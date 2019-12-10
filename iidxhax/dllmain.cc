// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "util/cmdline.h"
#include "util/fuzzy_search.h"

#include "minhook/hde64.h"
#include "minhook/include/MinHook.h"

#include "iidxhax/config.h"
#include "util/patch.h"
#include "util/xmlprop.hpp"

struct iidxhax_config config = {};

PSMAP_BEGIN(config_psmap, static)
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, omnimix,
                 "/iidxhax/system/omnimix")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, bms, "/iidxhax/system/bms")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, skip_camera_error,
                 "/iidxhax/system/skip_camera_error")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, lock_fps_enabled,
                 "/iidxhax/system/lock_fps/enabled")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_FLOAT, struct iidxhax_config, lock_fps_value,
                 "/iidxhax/system/lock_fps/value")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, monitor_adjust_enabled,
                 "/iidxhax/system/monitor_adjust/enabled")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_S32, struct iidxhax_config, monitor_adjust_value,
                 "/iidxhax/system/monitor_adjust/value")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, debug_mode,
                 "/iidxhax/system/debug_mode")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, patch_sse42,
                 "/iidxhax/system/patch_sse42")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, premium_free,
                 "/iidxhax/mode_select/premium_free")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, expert_mode,
                 "/iidxhax/mode_select/expert_mode")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, timer_freeze,
                 "/iidxhax/song_select/timer_freeze")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, premium_timer_freeze,
                 "/iidxhax/song_select/premium_timer_freeze")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, cursor_lock,
                 "/iidxhax/song_select/cursor_lock")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, unlock_all,
                 "/iidxhax/song_select/unlock_all")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, remove_song_select_color_banners,
                 "/iidxhax/song_select/remove_song_select_color_banners")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, all_video_preview,
                 "/iidxhax/song_select/all_video_preview")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, skip_decide_screen,
                 "/iidxhax/song_select/skip_decide_screen")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, cs_song_delay,
                 "/iidxhax/song/cs_song_delay")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, dark_mode,
                 "/iidxhax/song/dark_mode")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, hide_measure_lines,
                 "/iidxhax/song/hide_measure_lines")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, quick_retry,
                 "/iidxhax/song/quick_retry")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, all_notes_preview,
                 "/iidxhax/song/all_notes_preview")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, show_fs_total,
                 "/iidxhax/song/show_fs_total")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, top_left,
                 "/iidxhax/ticker_output/top_left")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, freeplay_ticker,
                 "/iidxhax/ticker_output/freeplay_ticker")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, notavailable_ticker,
                 "/iidxhax/ticker_output/notavailable_ticker")
PSMAP_MEMBER_REQ(PSMAP_PROPERTY_TYPE_BOOL, struct iidxhax_config, hide_all_text,
                 "/iidxhax/ticker_output/hide_all_text")
PSMAP_END

const int32_t MUSIC_DATA_BUFFER_SIZE = 0x200000;

void (*real_omnimix_patch3_hax)();
void (*real_omnimix_patch3_hax2)();
extern "C" void omnimix_patch3_hax();
__asm(".globl omnimix_patch3_hax\n"
      "omnimix_patch3_hax:\n"
      "  call [real_omnimix_patch3_hax]\n"
      "  inc dword ptr [r14+0x0c]\n"
      "  and byte ptr [r14+0x0d], 0x07\n"
      "  jmp [real_omnimix_patch3_hax2]\n");

void (*real_omnimix_patch_jbx)();
void omnimix_patch_jbx() {
    __asm("mov byte ptr [rbx+0x05], 'X'\n");

    real_omnimix_patch_jbx();
}

void (*real_omnimix_patch_jbz)();
void omnimix_patch_jbz() {
    __asm("mov byte ptr [rbx+0x05], 'Z'\n");

    real_omnimix_patch_jbz();
}

void (*real_omnimix_patch_musicdata)();
uint8_t musicdata_buffer[MUSIC_DATA_BUFFER_SIZE] = {};
void omnimix_patch_musicdata() {
    __asm("lea rax, [musicdata_buffer]\n"
          "  ret");
}

void (*real_omnimix_patch_musicdata_addr1)();
void omnimix_patch_musicdata_addr1() {
    __asm("lea rdx, [musicdata_buffer]\n");

    real_omnimix_patch_musicdata_addr1();
}

void (*real_omnimix_patch_musicdata_addr2)();
void omnimix_patch_musicdata_addr2() {
    __asm("lea rcx, [musicdata_buffer+0x10]\n");

    real_omnimix_patch_musicdata_addr2();
}

void (*real_omnimix_patch_musicdata_addr3)();
void omnimix_patch_musicdata_addr3() {
    __asm("lea rax, [musicdata_buffer+0x10]\n");

    real_omnimix_patch_musicdata_addr3();
}

static bool patch_omnimix() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    // Patch 1
    {
        fuzzy_search_task task;

        FUZZY_START(task, 7)
        FUZZY_CODE(task, 0, "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x40\x8B\xFA\x48\x8B\xD9\xE8", 16)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x8B\x93\xC8\x01\x00\x00\x48\x8B\xC8\xE8", 10)
        FUZZY_WILDCARD(task, 3, 4)
        FUZZY_CODE(task, 4, "\x84\xC0\x74", 3)
        FUZZY_WILDCARD(task, 5, 1)
        FUZZY_CODE(task, 6, "\xE8", 1)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find omnimix patch 1\n");
            return false;
        }

        pattern_offset = find_block(data, dllSize, &task, pattern_offset + 1);
        if (pattern_offset == -1) {
            printf("Couldn't find omnimix patch 1\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xC3", 1);
    }

    // Song limit patch
    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\xB9", 1)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x66\x39\x48\x08\x7F", 5)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find song limit patch for omnimix\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        int32_t new_song_count = 2000;
        patch_memory(patch_addr, (char *)&new_song_count, 4);
    }

    // Patch 3
    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\xe8", 1)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x41\xff\x46\x0c", 4)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find patch 3 for omnimix\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;

        real_omnimix_patch3_hax =
            (void (*)())((patch_addr + task.blocks[0].data[0].length + task.blocks[1].length) +
                         *(int32_t *)(patch_addr + 1));
        real_omnimix_patch3_hax2 =
            (void (*)())(patch_addr + task.blocks[0].data[0].length + task.blocks[1].length);

        patch_memory((uint64_t)real_omnimix_patch3_hax2, (char *)"\x90\x90\x90\x90", 4);
        MH_CreateHook((LPVOID)patch_addr, (LPVOID)omnimix_patch3_hax, NULL);
    }

    return true;
}

static bool patch_omni_x() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    // Music bin patch
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "music_data.bin", 14)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find music bin patch for omnimix\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        const char *new_music_bin = "music_omni.bin";
        patch_memory(patch_addr, (char *)new_music_bin, strlen(new_music_bin));
    }

    // JBX patch
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x0f\xb6\x4b\x04\xe8", 5)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find JBX patch for omnimix\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length - 1;
        MH_CreateHook((LPVOID)patch_addr, (LPVOID)omnimix_patch_jbx,
                      (void **)&real_omnimix_patch_jbx);
    }

    printf("Patched omnimix (music_omni.bin, song limit, rev=X, ");

    return true;
}

static bool patch_bms_z() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    // Music bin patch
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "/data/info/0/music_data.bin", 27)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"/datab/info/0/music_bms.bin", 27);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "/data/info/1/music_data.bin", 27)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"/datab/info/1/music_bms.bin", 27);
        }
    }

    // Course bin patch
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "/data/info/0/class_course_data.bin", 34)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"/datab/info/0/class_course_bms.bin", 34);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "/data/info/1/class_course_data.bin", 34)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"/datab/info/1/class_course_bms.bin", 34);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "/data/sound/system/0/", 21)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"/datab/sound/system/0/", 22);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "/data/sound/system/1/", 21)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"/datab/sound/system/1/", 22);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x69\x6E\x00/data/sound/", 15)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find /data/sound/ patch for bms\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x69\x6E\x00/datab/sound/", 16);
    }

    int64_t first_loc = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "dummy.wmv", 9)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "/data/movie/", 12)

        int64_t pattern_offset = find_block(data, 0x80, &task, first_loc);
        if (pattern_offset == -1) {
            printf("Couldn't find /data/movie/ patch for bms\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"/datab/movie/", 13);
    }

    // JBZ patch
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x0f\xb6\x4b\x04\xe8", 5)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find JBZ patch for bms\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length - 1;
        MH_CreateHook((LPVOID)patch_addr, (LPVOID)omnimix_patch_jbz,
                      (void **)&real_omnimix_patch_jbz);
    }

    printf("Patched bms (music_bms.bin, song limit, rev=Z, use datab folder, ");

    return true;
}

static bool patch_music_data() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    // Patch 0x180000 part 1
    int64_t size_patch_part1 = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x48\x63\xC8\x48\x81\xF9", 6)
        FUZZY_CODE(task, 1, "\x00\x00\x18\x00", 4)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find music_data.bin size patch 1\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory((uint64_t)patch_addr, (char *)&MUSIC_DATA_BUFFER_SIZE, 4);

        size_patch_part1 =
            pattern_offset + task.blocks[0].data[0].length + task.blocks[1].data[0].length;
    }

    // Patch 0x180000 part 2
    int64_t size_patch_part2 = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x00\x00\x18\x00", 4)

        int64_t pattern_offset = find_block(data, 0x100, &task, size_patch_part1);
        if (size_patch_part2 == -1) {
            printf("Couldn't find music_data.bin size patch 2 and buffer addr patch 1\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory((uint64_t)patch_addr, (char *)&MUSIC_DATA_BUFFER_SIZE, 4);

        MH_CreateHook((LPVOID)(patch_addr + 11), (LPVOID)omnimix_patch_musicdata_addr1,
                      (void **)&real_omnimix_patch_musicdata_addr1);
    }

    // Patch music_data.bin buffer
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x33\xC9\x4C\x8B\xC0\x80\x38\x49", 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find music_data.bin buffer patch\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_addr += *(int32_t *)(patch_addr - 4);

        MH_CreateHook((LPVOID)patch_addr, (LPVOID)omnimix_patch_musicdata,
                      (void **)&real_omnimix_patch_musicdata);
    }

    // Patch music_data.bin buffer addr 2
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x0F\xB7\x0C\x59\x66\x85\xC9", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find music_data.bin buffer addr 2 patch\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        MH_CreateHook((LPVOID)patch_addr, (LPVOID)omnimix_patch_musicdata_addr2,
                      (void **)&real_omnimix_patch_musicdata_addr2);
    }

    // Patch music_data.bin buffer addr 3
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x48\x03\xD1\x48\x8D\x04\x50", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find music_data.bin buffer addr 3 patch\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        MH_CreateHook((LPVOID)patch_addr, (LPVOID)omnimix_patch_musicdata_addr3,
                      (void **)&real_omnimix_patch_musicdata_addr3);
    }

    printf("increase buffer)\n");

    return true;
}

static bool patch_camera_error() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 4)
        FUZZY_CODE(task, 0, "\x84\xC0", 2)
        FUZZY_CODE(task, 1, "\x0f\x84", 2)
        FUZZY_WILDCARD(task, 2, 4)
        FUZZY_CODE(task, 3, "\x33\xD2\x48\x8D\x4C\x24\x40\x41\xB8\x00\x01\x00\x00", 13)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr + 1, (char *)"\x81", 1);
    }

    printf("Patched camera error\n");

    return true;
}

static bool patch_skip_monitor_check() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xF9\x3D\xB0\x04\x00\x00\x7C", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\xF9\x3D\xB0\x04\x00\x00\x7D", 7);

            printf("Patched skip monitor check\n");
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x3D\xB0\x04\x00\x00\x0F\x8C", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x3D\xB0\x04\x00\x00\x0F\x8D", 7);

            printf("Patched skip monitor check\n");
        }
    }

    return true;
}

static bool patch_fps(float fps) {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x00\x00\x70\x42\x00\x00\x00\x00\x22\x05\x93\x19\x01\x00\x00\x00", 16)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)&fps, 4);

        printf("Patched FPS: %f\n", *(float *)patch_addr);
    }

    return true;
}

static bool patch_monitor_adjust(int ms) {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0,
                   "\x66\x66\x30\x30\x30\x30\x66\x66\x00\x00\x00\x00\x00\x00\x00\x00"
                   "\x66"
                   "\x66\x66\x66\x66\x66\x66\x66\x00\x00\x00\x00\x00\x00\x00\x00",
                   32)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x0A", 1)

        int64_t pattern_offset = find_block(data, 0x45, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)&ms, 4);
            printf("Patched HD monitor adjust: %d ms\n", *(int32_t *)patch_addr);
        }
    }

    return true;
}

static bool patch_debug_mode() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\xE8", 1)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x84\xC0\xBB\x01\x00\x00\x00\xB9\x08\x00\x00\x00\x0F\x45\xD9", 15)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_addr += *(int32_t *)patch_addr;
        patch_addr += 4;

        patch_memory(patch_addr, (char *)"\x0c\x01", 2);
    }

    printf("Patched debug mode\n");

    return true;
}

static bool patch_premium_free() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    int64_t first_loc = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 6)
        FUZZY_CODE(task, 0, "\x83\xF9\x06\x74", 4)
        FUZZY_WILDCARD(task, 1, 1)
        FUZZY_CODE(task, 2, "\x83\xF9\x08", 3)
        FUZZY_CODE(task, 3, "\x75", 1)
        FUZZY_WILDCARD(task, 4, 1)
        FUZZY_CODE(task, 5, "\xE8", 1)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            printf("Couldn't find premium free loc 1\n");
            return false;
        }
    }

    int64_t second_loc = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xBA\x01\x00\x00\x00", 5)

        second_loc = find_block(data, 0x100, &task, first_loc);
        if (second_loc == -1) {
            printf("Couldn't find premium free loc 2\n");
            return false;
        }
    }

    int64_t third_loc = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x84\xC0", 2)
        FUZZY_CODE(task, 1, "\x74", 1)

        int64_t pattern_offset = find_block_back(data, 0x100, &task, second_loc);
        if (pattern_offset == -1) {
            printf("Couldn't find premium free loc 3\n");
            return false;
        }

        third_loc = pattern_offset + task.blocks[0].data[0].length + task.blocks[1].data[0].length;

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x84\xC0", 2)
        FUZZY_CODE(task, 1, "\x74", 1)

        int64_t pattern_offset = find_block(data, 0x100, &task, third_loc);
        if (pattern_offset == -1) {
            printf("Couldn't find premium free loc 3.5\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    int64_t fourth_loc = 0;
    bool found_fourth = false;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x77\xBE\x9F\x1A\x2F\xDD\x24\x06", 8)

        fourth_loc = find_block_back(data, 0x2000, &task, third_loc);
        if (fourth_loc != -1) {
            found_fourth = true;
        }
    }

    if (found_fourth) {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x84\xC0", 3)
        FUZZY_CODE(task, 1, "\x74", 1)

        int64_t pattern_offset = find_block(data, 0x100, &task, fourth_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
            patch_memory(patch_addr, (char *)"\xEB", 1);
        }
    }

    printf("Patched premium free\n");

    return true;
}

static bool patch_expert_mode() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 6)
        FUZZY_CODE(task, 0, "\x83\xF9\x06\x74", 4)
        FUZZY_WILDCARD(task, 1, 1)
        FUZZY_CODE(task, 2, "\x83\xF9\x08", 3)
        FUZZY_CODE(task, 3, "\x75", 1)
        FUZZY_WILDCARD(task, 4, 1)
        FUZZY_CODE(task, 5, "\xE8", 1)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            printf("Couldn't find expert mode patch\n");
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length +
                              task.blocks[1].length + task.blocks[2].data[0].length;
        patch_memory(patch_addr, (char *)"\xEB", 1);
    }

    printf("Patched expert mode\n");

    return true;
}

static bool patch_timer_freeze() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\x74", 1)
        FUZZY_WILDCARD(task, 1, 1)
        FUZZY_CODE(task, 2, "\xFF\x4B\x78\x8B\x4B\x78", 6)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xeb", 1);
    }

    printf("Patched timer freeze\n");

    return true;
}

static bool patch_premium_timer_freeze() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\x74", 1)
        FUZZY_WILDCARD(task, 1, 1)
        FUZZY_CODE(task, 2, "\xFF\xC8\x89\x41\x0C", 5)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\xeb", 1);
    }

    printf("Patched premium timer freeze\n");

    return true;
}

static bool patch_cursor_lock() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xB9\xE0\x94\xB2\x1E\x8D\x50\x06\xE8", 9)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x84\xc0", 2)
        FUZZY_CODE(task, 1, "\x74", 1)

        int64_t pattern_offset = find_block(data, 0x1000, &task, first_loc);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    printf("Patched cursor lock\n");

    return true;
}

static bool patch_unlock_all() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x39\x03", 2)
        FUZZY_CODE(task, 1, "\x74", 1)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x49\x63\xF8\x48\x8B\xDA\x83\xFF\x02", 9)
        FUZZY_CODE(task, 1, "\x74", 1)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    printf("Patched unlock all\n");

    return true;
}

static bool patch_remove_song_select_color_banners() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    bool patched = false;
    while (1) {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "listb_", 6)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            break;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"listb\x00", 6);
        patched = true;
    }

    if (patched) {
        printf("Patched remove song select color banners\n");
    }

    return patched;
}

static bool patch_all_video_preview() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x00\x00\x49\x8D\x8E\xD8\x01\x00\x00", 9)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x74", 1)

        int64_t pattern_offset = find_block(data, 0x20, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x90\x90", 2);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x05\x7C\x39\x0F\xB6", 5)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x05\x7C\x39\xEB\x2F", 5);
        }
    }

    printf("Patched video previews to all songs in select\n");
    return true;
}

static bool patch_skip_decide_screen() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    int64_t first_loc = 0;

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x41\x30\x48\xC7\x41\x38\x01\x00\x00\x00\x89\x41\x40\x48\x8B\xC1", 16)

        first_loc = find_block(data, dllSize, &task, 0);
        if (first_loc == -1) {
            return false;
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\xE8", 1)
        FUZZY_WILDCARD(task, 1, 1)
        FUZZY_CODE(task, 2, "\x00\x00\x00", 3)

        int64_t pattern_offset = find_block(data, 0x4A, &task, first_loc);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x90\x90\x90\x90\x90", 5);
        }
    }

    printf("Patched skip decide screen\n");

    return true;
}

static bool patch_cs_song_delay() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x81\xF9\x58\x02\x00\x00", 6)
        FUZZY_CODE(task, 1, "\x7D", 1)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    printf("Patched CS-style song start delay\n");

    return true;
}

static bool patch_dark_mode() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\xB9\x1A\x00\x00\x00\xE8", 6)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x84\xC0", 2)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length +
                              task.blocks[1].length + task.blocks[2].data[0].length;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    printf("Patched dark mode\n");

    return true;
}

static bool patch_hide_measure_lines() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 7)
        FUZZY_CODE(task, 0, "\x83\xF8\x04", 3)
        FUZZY_CODE(task, 1, "\x75", 1)
        FUZZY_WILDCARD(task, 2, 1)
        FUZZY_CODE(task, 3, "\x81", 1)
        FUZZY_WILDCARD(task, 4, 1)
        FUZZY_CODE(task, 5, "\xE2\x01\x00\x00\x77", 5)
        FUZZY_WILDCARD(task, 6, 1)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length;
        patch_memory(patch_addr, (char *)"\xEB", 1);
    }

    printf("Patched hide measure lines\n");

    return true;
}

static bool patch_quick_retry() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 7)
        FUZZY_CODE(task, 0, "\x40\x53\x48\x83\xEC\x20\x8B\xCA\x8B\xDA\xE8", 11)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x84\xC0\x74", 3)
        FUZZY_WILDCARD(task, 3, 1)
        FUZZY_CODE(task, 4, "\x8B\xCB\x48\x83\xC4\x20\x5B\xE9", 8)
        FUZZY_WILDCARD(task, 5, 4)
        FUZZY_CODE(task, 6, "\x32\xC0\x48\x83\xC4\x20\x5B\xC3", 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset + task.blocks[0].data[0].length +
                              task.blocks[1].length + task.blocks[2].data[0].length +
                              task.blocks[3].length + task.blocks[4].data[0].length +
                              task.blocks[5].length;
        patch_memory(patch_addr, (char *)"\x34\x01", 2);
    }

    printf("Patched quick retry\n");

    return true;
}

static bool patch_all_notes_preview() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    bool patched = false;
    while (1) {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x05\x00\x00\x00\x84\xC0", 6)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            break;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;

        patch_memory(patch_addr, (char *)"\x0C\x00\x00\x00\x84\xC0", 6);
        patched = true;
    }

    if (patched) {
        printf("Patched starting notes preview up to 12s\n");
    }

    return patched;
}

static bool patch_show_fs_total() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xA0\x00\x00\x00\x74\x57\xE8", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\xA0\x00\x00\x00\x75\x57\xE8", 7);
        }
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x90\x00\x00\x00\x74\x57\xE8", 7)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset != -1) {
            uint64_t patch_addr = (int64_t)data + pattern_offset;
            patch_memory(patch_addr, (char *)"\x90\x00\x00\x00\x75\x57\xE8", 7);

            printf("Patched fast/slow total opposite toggle\n");
        }
    }

    return true;
}

int64_t get_absolute_ticker_offset() {
    int64_t absolute_ticker_offset = 0;

    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    // Get ticker absolute address from relative address
    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\x41\xB8\x00\x02\x00\x00\x48\x8D\x0D", 9)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return -1;
        }

        absolute_ticker_offset =
            (int64_t)data + pattern_offset +
            *(int32_t *)(data + pattern_offset + task.blocks[0].data[0].length) +
            task.blocks[0].data[0].length + 4;
    }

    return absolute_ticker_offset;
}

static bool patch_ticker_top_left() {
    int64_t absolute_ticker_offset = get_absolute_ticker_offset();

    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 5)
        FUZZY_CODE(task, 0, "\x48\x89\xAC\x24\x90\x00\x00\x00\x48\x8D\x2D", 11)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x48\x89\xB4\x24\x98\x00\x00\x00\x48\x8D\x35", 11)
        FUZZY_WILDCARD(task, 3, 4)
        FUZZY_CODE(task, 4, "\x0F\x29\x74\x24\x70", 5)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint32_t pattern_size =
            task.blocks[0].data[0].length + task.blocks[1].length + task.blocks[2].data[0].length;
        uint32_t relative_ticker_offset =
            absolute_ticker_offset - ((int64_t)data + pattern_offset + pattern_size + 4);
        uint64_t patch_addr = (int64_t)data + pattern_offset + pattern_size;
        patch_memory(patch_addr, (char *)&relative_ticker_offset, 4);
    }

    {
        fuzzy_search_task task;

        FUZZY_START(task, 5)
        FUZZY_CODE(task, 0, "\x84\xC0", 2)
        FUZZY_WILDCARD(task, 1, 2)
        FUZZY_CODE(task, 2, "\x48\x8B\x05", 3)
        FUZZY_WILDCARD(task, 3, 4)
        FUZZY_CODE(task, 4, "\x33\xD2\x45\x33\xC9\x45\x33\xC0", 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint32_t pattern_size = task.blocks[0].data[0].length;
        uint64_t patch_addr = (int64_t)data + pattern_offset + pattern_size;
        patch_memory(patch_addr, (char *)"\x90\x90", 2);
    }

    printf("Patched ticker top left\n");

    return true;
}

static bool patch_freeplay_ticker() {
    int64_t absolute_ticker_offset = get_absolute_ticker_offset();

    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\xBA\xF4\x04\x00\x00\x48\x8D\x0D", 8)
        FUZZY_WILDCARD(task, 1, 4)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint32_t pattern_size = task.blocks[0].data[0].length;
        uint32_t relative_ticker_offset =
            absolute_ticker_offset - ((int64_t)data + pattern_offset + pattern_size + 4);
        uint64_t patch_addr = (int64_t)data + pattern_offset + pattern_size;
        patch_memory(patch_addr, (char *)&relative_ticker_offset, 4);
    }

    printf("Patched FREE PLAY ticker\n");

    return true;
}

static bool patch_notavailable_ticker() {
    int64_t absolute_ticker_offset = get_absolute_ticker_offset();

    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\x48\x8D\x05", 3)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x44\x8B\xCA\x48\x89\x44\x24\x38", 8)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint32_t pattern_size = task.blocks[0].data[0].length;
        uint32_t relative_ticker_offset =
            absolute_ticker_offset - ((int64_t)data + pattern_offset + pattern_size + 4);
        uint64_t patch_addr = (int64_t)data + pattern_offset + pattern_size;
        patch_memory(patch_addr, (char *)&relative_ticker_offset, 4);
    }

    printf("Patched NOT AVAILABLE ticker\n");

    return true;
}

static bool patch_hide_all_text() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(
            task, 0,
            "\x46\x52\x45\x45\x20\x50\x4C\x41\x59\x00\x00\x00\x00\x00\x00\x00\x43\x52\x45\x44\x49"
            "\x54\x3A\x20\x25\x64\x20\x43\x4F\x49\x4E\x3A\x20\x25\x64\x20\x2F\x20\x25\x64\x00\x00"
            "\x00\x00\x00\x00\x00\x00\x43\x52\x45\x44\x49\x54\x3A\x20\x25\x64\x00\x00\x00\x00\x00"
            "\x00\x50\x41\x53\x45\x4C\x49\x3A\x20\x4E\x4F\x54\x20\x41\x56\x41\x49\x4C\x41\x42\x4C"
            "\x45\x00\x00\x00\x45\x58\x54\x52\x41\x20\x50\x41\x53\x45\x4C\x49\x3A\x20\x25\x64\x00"
            "\x00\x00\x00\x00\x00\x00\x00\x45\x58\x54\x52\x41\x20\x50\x41\x53\x45\x4C\x49\x3A\x20"
            "\x25\x73\x00\x00\x00\x00\x00\x00\x00\x00\x50\x41\x53\x45\x4C\x49\x3A\x20\x25\x64\x00"
            "\x00\x00\x00\x00\x00\x50\x41\x53\x45\x4C\x49\x3A\x20\x25\x73\x00\x00\x00\x00\x00\x00"
            "\x50\x41\x53\x45\x4C\x49\x3A\x20\x2A\x2A\x2A\x2A\x2A\x2A\x00\x00\x20\x2B\x20\x25\x64"
            "\x00\x00\x00\x20\x2B\x20\x25\x73\x00\x00\x00\x50\x41\x53\x45\x4C\x49\x3A\x20\x4E\x4F"
            "\x20\x41\x43\x43\x4F\x55\x4E\x54\x00\x00\x00\x00\x00\x00\x49\x4E\x53\x45\x52\x54\x20"
            "\x43\x4F\x49\x4E\x5B\x53\x5D\x00\x00",
            240)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(
            patch_addr,
            (char *)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
            240);
    }

    printf("Patched hiding all bottom text\n");

    return true;
}

static bool patch_sse42() {
    DWORD dllSize = 0;
    char *data = getDllData("bm2dx.dll", &dllSize);

    {
        fuzzy_search_task task;

        FUZZY_START(task, 1)
        FUZZY_CODE(task, 0, "\xF3\x45\x0F\xB8\xD3", 5)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        uint64_t patch_addr = (int64_t)data + pattern_offset;
        patch_memory(patch_addr, (char *)"\x90\x90\x90", 3);
    }

    printf("Patched SSE 4.2 instructions for poorbois (UPGRADE YOUR CPU!)\n");

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

        _load_config("iidxhax.xml", &config, config_psmap);

        int argc;
        char **argv;

        args_recover(&argc, &argv);

        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], "omni") == 0) {
                config.omnimix = true;
                config.bms = false;
            } else if (strcmp(argv[i], "bms") == 0) {
                config.omnimix = false;
                config.bms = true;
            }
        }

        if (config.omnimix) {
            patch_omnimix();
            patch_omni_x();
            patch_music_data();
        }

        if (config.bms) {
            patch_omnimix();
            patch_bms_z();
            patch_music_data();
        }

        if (config.skip_camera_error) {
            patch_camera_error();
        }

        if (config.lock_fps_enabled) {
            patch_skip_monitor_check();
            patch_fps(config.lock_fps_value);
        }

        if (config.monitor_adjust_enabled) {
            patch_monitor_adjust(config.monitor_adjust_value);
        }

        if (config.debug_mode) {
            patch_debug_mode();
        }

        if (config.premium_free) {
            patch_premium_free();
        }

        if (config.expert_mode) {
            patch_expert_mode();
        }

        if (config.timer_freeze) {
            patch_timer_freeze();
        }

        if (config.premium_timer_freeze) {
            patch_premium_timer_freeze();
        }

        if (config.cursor_lock) {
            patch_cursor_lock();
        }

        if (config.unlock_all) {
            patch_unlock_all();
        }

        if (config.remove_song_select_color_banners) {
            patch_remove_song_select_color_banners();
        }

        if (config.all_video_preview) {
            patch_all_video_preview();
        }

        if (config.skip_decide_screen) {
            patch_skip_decide_screen();
        }

        if (config.cs_song_delay) {
            patch_cs_song_delay();
        }

        if (config.dark_mode) {
            patch_dark_mode();
        }

        if (config.hide_measure_lines) {
            patch_hide_measure_lines();
        }

        if (config.quick_retry) {
            patch_quick_retry();
        }

        if (config.all_notes_preview) {
            patch_all_notes_preview();
        }

        if (config.show_fs_total) {
            patch_show_fs_total();
        }

        if (config.top_left) {
            patch_ticker_top_left();
        }

        if (config.freeplay_ticker) {
            patch_freeplay_ticker();
        }

        if (config.notavailable_ticker) {
            patch_notavailable_ticker();
        }

        if (config.hide_all_text) {
            patch_hide_all_text();
        }

        if (config.patch_sse42) {
            patch_sse42();
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

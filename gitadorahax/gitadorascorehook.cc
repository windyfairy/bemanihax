// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on

#include <atomic>
#include "mingw.future.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "imports/avs.h"
#include "imports/avs-ea3.h"

#include "minhook/include/MinHook.h"

#include "util/cmdline.h"
#include "util/fuzzy_search.h"
#include "util/patch.h"

static const char *xrpcEncodingType = "SHIFT_JIS";
static const char *xrpcEndpointName = "fakeeascore";
static const char *servicesUrlEndpoint = "fakeeascore";

static bool initializeSuccess = false;
static bool isUnlocked[2] = {false};
static bool isQueued[2] = {false};
static uint8_t *playerCardIds[2] = {nullptr, nullptr};
static bool score_seen[2][10] = {{false}, {false}};

static uint64_t scoreMemoryAddress = 0;
static uint64_t scoreMemoryBaseAddress = 0;
static uint32_t scoreEntrySize = 0;
static uint32_t scorePlayerSize = 0;
static uint64_t playerTimingMemoryAddress = 0;
static uint64_t playerTimingDrumHookAddress = 0;
static uint64_t playerTimingGuitarHookAddress = 0;
static uint64_t soundArchiveOffset = 0;

int32_t timingPlayerId = 0;
int32_t timingPlayerJudge = 0;
int32_t timingPlayer[2][3] = {{0, 0, 0}, {0, 0, 0}};

static void (*real_mo_manager_set_music_play_info)(void *);
static void (*real_mo_manager_sync_set_music_select_info)(void *, int);
static void (*real_mo_manager_end)();
static int32_t (*real_cardunit_key_get)(uint32_t deviceNum);
static int32_t (*real_cardunit_card_read2)(int32_t, uint8_t *const, int32_t *);

typedef void(__stdcall *CGf2DDrawScreen)(int, int, int, int);
typedef void(__stdcall *CGfFontSelectFontType)(int);
typedef void(__stdcall *CGfFontSelectEncodingType)(int);
typedef void(__stdcall *CGfFontOrigin)(int);
typedef void(__stdcall *CGfFontScale)(float, float);
typedef void(__stdcall *CGfFontRotation)(float);
typedef void(__stdcall *CGfFontPosition)(float, float);
typedef void(__stdcall *CGfFontColor3ub)(uint8_t, uint8_t, uint8_t);
typedef void(__stdcall *CGfFontShadow)(int);
typedef void(__stdcall *CGfFontGetPosition)(float *, float *);
typedef void(__stdcall *CGfFontPuts)(const char *);
typedef int(__stdcall *CSysWindowGetWidth)();
typedef int(__stdcall *CSysWindowGetHeight)();
typedef int(__stdcall *CSysSettingGetParam)(const char *);
typedef char *(__stdcall *CSysCodeGetSoftwareSpecificationCode)();

CGf2DDrawScreen gf2DDrawScreen;
CGfFontSelectFontType gfFontSelectFontType;
CGfFontSelectEncodingType gfFontSelectEncodingType;
CGfFontOrigin gfFontOrigin;
CGfFontScale gfFontScale;
CGfFontRotation gfFontRotation;
CGfFontPosition gfFontPosition;
CGfFontColor3ub gfFontColor3ub;
CGfFontShadow gfFontShadow;
CGfFontGetPosition gfFontGetPosition;
CGfFontPuts gfFontPuts;
CSysWindowGetWidth sys_window_get_width;
CSysWindowGetHeight sys_window_get_height;
CSysSettingGetParam sys_setting_get_param;
CSysCodeGetSoftwareSpecificationCode sys_code_get_software_specification_code;

struct fakeeascore_verify_ctx {
    uint8_t data[0x1000];
    uint8_t carddata[8];
    uint32_t timing_slow;
    uint32_t timing_fast;
    int32_t timing_total;
    uint32_t len;
};

static std::atomic<int32_t> xrpc_req_status;

enum fakeeascore_state {
    FAKEEASCORE_STATE_INDETERMINATE = 0,
    FAKEEASCORE_STATE_OKAY,
    FAKEEASCORE_STATE_SERVER_ERROR,
    FAKEEASCORE_STATE_UNKNOWN_ERROR,
};

void draw_text(const char *text, float x, float y, uint8_t r, uint8_t g, uint8_t b) {
    gf2DDrawScreen(0, 0, sys_window_get_width(), sys_window_get_height());
    gfFontSelectFontType(0);
    gfFontSelectEncodingType(4);
    gfFontOrigin(3);
    gfFontColor3ub(r, g, b);
    gfFontShadow(0);
    gfFontScale(1.5, 1.0);
    gfFontRotation(0.0);
    gfFontPosition(x, y);
    gfFontPuts(text);
}

static bool find_required_offsets(uint64_t *struct_base_offset, uint64_t *struct_offset,
                                  uint32_t *struct_size, uint32_t *player_struct_size,
                                  uint64_t *player_timing_offset,
                                  uint64_t *player_timing_drum_hook_offset,
                                  uint64_t *player_timing_guitar_hook_offset,
                                  uint64_t *sound_archive_offset) {
    DWORD dllSize = 0;
    char *data = getDllData("game.dll", &dllSize);

    // Pattern 1 to get base address: 48 8d 2d ?? ?? ?? ?? 4c 8d 25 ?? ?? ?? ?? 66 66 90 80
    int64_t pattern_a_offset = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 5)
        FUZZY_CODE(task, 0, "\x48\x8d\x2d", 3)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x4c\x8d\x25", 3)
        FUZZY_WILDCARD(task, 3, 4)
        FUZZY_CODE(task, 4, "\x66\x66\x90\x80", 4)

        pattern_a_offset = find_block(data, dllSize, &task, 0);
        if (pattern_a_offset == -1) {
            return false;
        }

        uint64_t rel_offset = *(uint32_t *)(data + pattern_a_offset + FUZZY_SIZE(task, 0) +
                                            FUZZY_SIZE(task, 1) + FUZZY_SIZE(task, 2));
        *struct_base_offset = rel_offset;
        *struct_offset = (uint64_t)data + pattern_a_offset + rel_offset + 14;
    }

    // Pattern 2 to get struct size: 48 69 ?? ?? ?? ?? ?? <- Should be within a few instructions
    // after the first pattern
    int64_t pattern_b_offset = 0;
    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\x48\x69", 2)
        FUZZY_WILDCARD(task, 1, 1)
        FUZZY_WILDCARD(task, 2, 4)

        pattern_b_offset = find_block(data, dllSize, &task, pattern_a_offset);
        if (pattern_b_offset == -1) {
            return false;
        }

        *struct_size =
            *(uint32_t *)(data + pattern_b_offset + FUZZY_SIZE(task, 0) + FUZZY_SIZE(task, 1));
    }

    // Pattern 3 to get difference between player 1 and player 2 structs: 48 81 c6 ?? ?? ?? ?? 83 FF
    // 02
    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\x48\x81\xc6", 3)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x83\xff\x02", 3)

        int64_t pattern_c_offset = find_block(data, dllSize, &task, pattern_b_offset);
        if (pattern_c_offset == -1) {
            return false;
        }

        *player_struct_size =
            *(uint32_t *)(data + pattern_c_offset + task.blocks[0].data[0].length);
    }

    // Pattern 4 to get offset that contains timing data for each player
    {
        fuzzy_search_task task;

        FUZZY_START(task, 2)
        FUZZY_CODE(task, 0, "\x83\xFD\x04\x41\x0F\x45\xC6\x89\x05", 9)
        FUZZY_WILDCARD(task, 1, 4)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        *player_timing_drum_hook_offset =
            (uint64_t)(data + pattern_offset + FUZZY_SIZE(task, 0) + FUZZY_SIZE(task, 1));
        *player_timing_offset =
            *(uint32_t *)(data + pattern_offset + task.blocks[0].data[0].length) +
            *player_timing_drum_hook_offset;
    }

    // Pattern 5 to get address to hook to update timing stats
    {
        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\x44\x8B\xB4\x24", 4)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(task, 2, "\x41\x8B\xCE", 3)

        int64_t pattern_offset = find_block(data, dllSize, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        *player_timing_guitar_hook_offset = (int64_t)(data + pattern_offset);
    }

    // Pattern 6 to get the va3 archive to play a sound when you successfully register a scorecard
    // press
    {
        DWORD dllSize2 = 0;
        char *data2 = getDllData("libshare-pj.dll", &dllSize2);

        fuzzy_search_task task;

        FUZZY_START(task, 3)
        FUZZY_CODE(task, 0, "\xBB\x14\x00\x00\x00\x48\x8B\x0D ", 8)
        FUZZY_WILDCARD(task, 1, 4)
        FUZZY_CODE(
            task, 2,
            "\x89\x74\x24\x60\x89\x74\x24\x58\x89\x74\x24\x50\x48\x89\x74\x24\x48\x48\x89\x74\x24"
            "\x40\x89\x74\x24\x38\x45\x33\xC0\xC6\x44\x24\x30\xFF\x41\x8D\x50\x0F\x41\xB1\x7F",
            41)

        int64_t pattern_offset = find_block(data2, dllSize2, &task, 0);
        if (pattern_offset == -1) {
            return false;
        }

        int base_offset =
            (uint64_t)(data2 + pattern_offset + FUZZY_SIZE(task, 0) + FUZZY_SIZE(task, 1));
        *sound_archive_offset =
            *(uint32_t *)(data2 + pattern_offset + task.blocks[0].data[0].length) + base_offset;
    }

    initializeSuccess = true;

    return true;
}

void timing_stats_internal(int playerId, int judge) {
    int timing = *(int32_t *)(playerTimingMemoryAddress + ((playerId * 5) * 8));

    if (judge == 4 || (timing > -3 && timing < 3)) {
        // Judge 4 = missed note, which is not counted
        // Timing between -3 > x > 3 is displayed in-game as 0.000, so not counted
        return;
    }

    timingPlayer[playerId][timing < 0 ? 0 : 1] += 1;
    timingPlayer[playerId][2] += timing * 3;

    // printf("Player ID: %d %d\n", playerId, timing);
}

extern "C" void timing_stats_drum_hook_internal();
void timing_stats_drum_hook_internal() {
    __asm("mov [timingPlayerJudge], ebp\n");

    timing_stats_internal(0, timingPlayerJudge);
}

void (*real_timing_stats_drum_hook)();
extern "C" void timing_stats_drum_hook();
__asm(".globl timing_stats_drum_hook\n"
      "timing_stats_drum_hook:\n"
      "  push rax\n"
      "  push rcx\n"
      "  push rdx\n"
      "  push rbx\n"
      "  push rbp\n"
      "  push rsi\n"
      "  push rdi\n"
      "  push r8\n"
      "  push r9\n"
      "  push r10\n"
      "  push r11\n"
      "  push r12\n"
      "  push r13\n"
      "  push r14\n"
      "  push r15\n"
      "  call timing_stats_drum_hook_internal\n"
      "  pop r15\n"
      "  pop r14\n"
      "  pop r13\n"
      "  pop r12\n"
      "  pop r11\n"
      "  pop r10\n"
      "  pop r9\n"
      "  pop r8\n"
      "  pop rdi\n"
      "  pop rsi\n"
      "  pop rbp\n"
      "  pop rbx\n"
      "  pop rdx\n"
      "  pop rcx\n"
      "  pop rax\n"
      "  jmp [real_timing_stats_drum_hook]\n");

extern "C" void timing_stats_guitar_hook_internal();
void timing_stats_guitar_hook_internal() {
    __asm("mov [timingPlayerId], edi\n");
    __asm("mov [timingPlayerJudge], esi\n");

    timing_stats_internal(timingPlayerId, timingPlayerJudge);
}

void (*real_timing_stats_guitar_hook)();
extern "C" void timing_stats_guitar_hook();
__asm(".globl timing_stats_guitar_hook\n"
      "timing_stats_guitar_hook:\n"
      "  push rax\n"
      "  push rcx\n"
      "  push rdx\n"
      "  push rbx\n"
      "  push rbp\n"
      "  push rsi\n"
      "  push rdi\n"
      "  push r8\n"
      "  push r9\n"
      "  push r10\n"
      "  push r11\n"
      "  push r12\n"
      "  push r13\n"
      "  push r14\n"
      "  push r15\n"
      "  call timing_stats_guitar_hook_internal\n"
      "  pop r15\n"
      "  pop r14\n"
      "  pop r13\n"
      "  pop r12\n"
      "  pop r11\n"
      "  pop r10\n"
      "  pop r9\n"
      "  pop r8\n"
      "  pop rdi\n"
      "  pop rsi\n"
      "  pop rbp\n"
      "  pop rbx\n"
      "  pop rdx\n"
      "  pop rcx\n"
      "  pop rax\n"
      "  jmp [real_timing_stats_guitar_hook]\n");

/* xrpc uploader callbacks */
static bool __cdecl fakeeascore_xrpc_init(void *shmem, char *buffer) { return true; }

static bool __cdecl fakeeascore_xrpc_sender(void *shmem, struct property_node *node) {
    struct fakeeascore_verify_ctx *ctx = (struct fakeeascore_verify_ctx *)shmem;

    property_node_create(NULL, node, PROPERTY_TYPE_U64, "offset", scoreMemoryBaseAddress);
    property_node_create(NULL, node, PROPERTY_TYPE_U32, "slen1", scoreEntrySize);
    property_node_create(NULL, node, PROPERTY_TYPE_U32, "slen2", scorePlayerSize);
    property_node_create(NULL, node, PROPERTY_TYPE_BIN, "card", ctx->carddata, 8);
    property_node_create(NULL, node, PROPERTY_TYPE_BIN, "data", ctx->data, ctx->len);
    property_node_create(NULL, node, PROPERTY_TYPE_U32, "timing_slow", ctx->timing_slow);
    property_node_create(NULL, node, PROPERTY_TYPE_U32, "timing_fast", ctx->timing_fast);
    property_node_create(NULL, node, PROPERTY_TYPE_S32, "timing_total", ctx->timing_total);

    return true;
}

static bool __cdecl fakeeascore_xrpc_receiver(void *shmem, struct property_node *node) {
    return true;
}

/* xrpc uploader methods */
static struct xrpc_method fakeeascore_xrpc_methods[] = {
    {
        .xrpc_meth_name = "upload",
        .xrpc_cb_init = fakeeascore_xrpc_init,
        .xrpc_cb_sender = fakeeascore_xrpc_sender,
        .xrpc_cb_receiver = fakeeascore_xrpc_receiver,
        .crypt_level = true,
        .padding_00 = 0,
        .use_xrpc11 = false,
        .use_esign = false,
        .use_ssl = true,
        .compress_type = false,
        .method_type = HTTPAC_HTTP10,
        .padding_01 = 0,
    },
    {
        .xrpc_meth_name = NULL,
    }};

static int fakeeascore_xrpc_finish_callback(void *buffer, struct xrpc_status status, void *param) {
    int16_t xrpc_status = status.status;
    int16_t xrpc_status_code = status.status_code;

    switch (xrpc_status) {
    case XRPC_STATUS_SERVER_RESPONSE_ERROR:
    case XRPC_STATUS_SERVER_FAULT_ERROR:
        printf("%s: Encountered internal error on callback.\n", __func__);
        std::atomic_store_explicit(&xrpc_req_status, (int)FAKEEASCORE_STATE_SERVER_ERROR,
                                   std::memory_order::memory_order_release);
        return 0;

    default:
        break;
    }

    if (xrpc_status_code != XRPC_OK) {
        printf("%s: Encountered server error on callback.\n", __func__);
        std::atomic_store_explicit(&xrpc_req_status, (int)FAKEEASCORE_STATE_SERVER_ERROR,
                                   std::memory_order::memory_order_release);
        return 0;
    }

    printf("%s: Port connection was okay.\n", __func__);
    std::atomic_store_explicit(&xrpc_req_status, (int)FAKEEASCORE_STATE_OKAY,
                               std::memory_order::memory_order_release);
    return 0;
}

static bool fakeeascore_xrpc_send_score_card(unsigned char *data, char *cardData, uint64_t len,
                                             int32_t playerNum, int32_t timing_slow,
                                             int32_t timing_fast, int32_t timing_total) {
    static struct fakeeascore_verify_ctx fakeeascore_ctx[2];
    const size_t sz_xrpc_buf = 256 * 1024;
    struct xrpc_handle *xrpc_handle;
    uint32_t delayed = 0;
    uint32_t req_status;
    bool ok = true;

    /* Reset xrpc request finished state */
    memset(&fakeeascore_ctx[playerNum], 0, sizeof(fakeeascore_ctx[playerNum]));
    std::atomic_store_explicit(&xrpc_req_status, 0, std::memory_order::memory_order_release);

    fakeeascore_ctx[playerNum].len = len;

    if (cardData != nullptr) {
        memcpy(fakeeascore_ctx[playerNum].carddata, cardData, 8);
    }

    memcpy(fakeeascore_ctx[playerNum].data, data, len);
    fakeeascore_ctx[playerNum].timing_slow = timing_slow;
    fakeeascore_ctx[playerNum].timing_fast = timing_fast;
    fakeeascore_ctx[playerNum].timing_total = timing_total;

    /* Create the xrpc handle */
    xrpc_handle = ea3_xrpc_new(sz_xrpc_buf, xrpcEncodingType, 0);
    ea3_xrpc_apply(xrpc_handle, "fakeeascore.upload", &fakeeascore_ctx[playerNum],
                   fakeeascore_xrpc_finish_callback, NULL, NULL);
    ea3_xrpc_destroy(xrpc_handle);

    /* Wait for the xrpc request to finish */
    while ((req_status = atomic_load_explicit(&xrpc_req_status, std::memory_order_acquire)) !=
           FAKEEASCORE_STATE_INDETERMINATE) {
        avs_thread_delay(100, 0);

        delayed += 100;
        if (delayed >= 3000) {
            break;
        }
    }

    /* Log information if the request failed. */
    if (req_status != FAKEEASCORE_STATE_OKAY) {
        printf("Failed to call fakeeascore.upload: error 0x%08x.\n", req_status);
        ok = false;
    }

    return ok;
}

bool send_scorebot_request(int32_t playerNum, bool send) {
    if (!initializeSuccess) {
        // Is this even really necessary? Oh well, just in case.
        if (!find_required_offsets(&scoreMemoryBaseAddress, &scoreMemoryAddress, &scoreEntrySize,
                                   &scorePlayerSize, &playerTimingMemoryAddress,
                                   &playerTimingDrumHookAddress, &playerTimingGuitarHookAddress,
                                   &soundArchiveOffset)) {
            return false;
        }
    }

    // TODO: Add some way to lock code here so we don't mess up isUnlocked

    // Find the last song in the list and send that to scorebot
    uint8_t *moduleBase = (uint8_t *)(scoreMemoryAddress + (playerNum * scorePlayerSize));

    int lastStageNum = 0;
    for (int stageNum = 0; stageNum < 10; stageNum++) {
        if (*moduleBase == 0x01 && !score_seen[playerNum][stageNum]) {
            lastStageNum = stageNum;
            score_seen[playerNum][stageNum] = true;
        }

        moduleBase += scoreEntrySize;
    }

    moduleBase = (uint8_t *)(scoreMemoryAddress + (playerNum * scorePlayerSize) +
                             (lastStageNum * scoreEntrySize));

    if (*moduleBase == 0x01 && *(moduleBase + 1) == 0x01) // Passed song
    {
        isUnlocked[playerNum] = false;

        if (send && playerCardIds[playerNum] != nullptr) {
            int32_t timing_slow = timingPlayer[playerNum][0];
            int32_t timing_fast = timingPlayer[playerNum][1];
            int32_t timing_total = timingPlayer[playerNum][2];

            char cardData[8] = {0};
            memcpy(cardData, playerCardIds[playerNum], 8);

            // Found last song entry, send to scorebot
            // If someone wants to come along and make a Discord scorebot thing like
            // exists for IIDX, just replace fakeeascore_xrpc_send_score_card with
            // the Discord code
            std::async(std::launch::async, [moduleBase, cardData, playerNum, timing_slow,
                                            timing_fast, timing_total]() {
                fakeeascore_xrpc_send_score_card(moduleBase, (char*)&cardData, scoreEntrySize, playerNum,
                                                 timing_slow, timing_fast, timing_total);
            });
        }
    }

    // Reset timing stats
    timingPlayer[playerNum][0] = 0;
    timingPlayer[playerNum][1] = 0;
    timingPlayer[playerNum][2] = 0;

    return true;
}

void my_mo_manager_set_music_play_info(void *param) {
    isUnlocked[0] = true;
    isUnlocked[1] = true;

    real_mo_manager_set_music_play_info(param);
}

void my_mo_manager_sync_set_music_select_info(void *param1, int param2) {
    // Send score data after stage
    for (int playerId = 0; playerId < 2; playerId++) {
        send_scorebot_request(playerId, isQueued[playerId]);
        isQueued[playerId] = false;
        isUnlocked[playerId] = false;
    }

    return real_mo_manager_sync_set_music_select_info(param1, param2);
}

void my_mo_manager_end() {
    // Send score data once game ends
    for (int playerId = 0; playerId < 2; playerId++) {
        send_scorebot_request(playerId, isQueued[playerId]);
        isQueued[playerId] = false;
        isUnlocked[playerId] = false;
        playerCardIds[playerId] = nullptr;

        for (int scoreIdx = 0; scoreIdx < 10; scoreIdx++) {
            score_seen[playerId][scoreIdx] = false;
        }
    }

    return real_mo_manager_end();
}

int32_t my_cardunit_key_get(uint32_t deviceNum) {
    int32_t result = real_cardunit_key_get(deviceNum);

    if (isUnlocked[deviceNum] && result == 0) {
        // Play sound to let the player know that their scorecard is being sent
        typedef int64_t(__stdcall * CBmsdVoicePlayPitch)(
            int64_t archiveInfo, int32_t soundId, uint64_t a3, uint8_t volume, uint8_t pan,
            int32_t a6, int32_t a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, int32_t a12,
            int32_t a13, int32_t a14, int32_t a15, int32_t a16, int32_t a17);
        CBmsdVoicePlayPitch bmsd_voice_play_pitch = reinterpret_cast<CBmsdVoicePlayPitch>(
            ::GetProcAddress(LoadLibraryA("libbmsd.dll"), "bmsd_voice_play_pitch"));

        if (bmsd_voice_play_pitch != nullptr) {
            bmsd_voice_play_pitch(*(int64_t *)soundArchiveOffset, 52, 0, 0x7f, 0x40, 0x14, 0xff, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0);
        }

        // 0 Pressed on keypad, send scorebot request
        isQueued[deviceNum] = true;
    }

    return result;
}

int32_t my_cardunit_card_read2(int32_t deviceNum, uint8_t *const output, int32_t *cardType) {
    int32_t result = real_cardunit_card_read2(deviceNum, output, cardType);

    if (*cardType != 0) {
        playerCardIds[deviceNum] = output;
    }

    return result;
}

void draw_stats(float x, float y, int timing_slow, int timing_fast, int timing_total) {
    int t = timing_slow + timing_fast;
    double average = t > 0 ? timing_total / 1000.0f / t : 0.0f;

    char buf[64] = {0};
    snprintf(buf, 64, "S:%d/F:%d/A:%s%0.3f", timing_slow, timing_fast, average < 0.0f ? "" : "+",
             average);

    // gfFontShadow is broken or I just can't figure out how to use it properly, so fake shadows
    draw_text(buf, x - 1, y, 0, 0, 0);
    draw_text(buf, x - 1, y - 1, 0, 0, 0);
    draw_text(buf, x, y - 1, 0, 0, 0);
    draw_text(buf, x + 1, y - 1, 0, 0, 0);
    draw_text(buf, x + 1, y, 0, 0, 0);
    draw_text(buf, x + 1, y + 1, 0, 0, 0);
    draw_text(buf, x, y + 1, 0, 0, 0);
    draw_text(buf, x, y, 255, 255, 255);
}

void disp_paseli_step() {
    draw_stats(240.0f, 694.0f, timingPlayer[0][0], timingPlayer[0][1], timingPlayer[0][2]);

    if (sys_setting_get_param("VER_MACHINE") != 0x3000 &&
        strncmp(sys_code_get_software_specification_code(), "B", 1) != 0) {
        draw_stats(1050.0f, 694.0f, timingPlayer[1][0], timingPlayer[1][1], timingPlayer[1][2]);
    }
}

void scorehook_init(bool scorecard_hook, bool display_timing_stats) {
    if (!find_required_offsets(&scoreMemoryBaseAddress, &scoreMemoryAddress, &scoreEntrySize,
                               &scorePlayerSize, &playerTimingMemoryAddress,
                               &playerTimingDrumHookAddress, &playerTimingGuitarHookAddress,
                               &soundArchiveOffset)) {
        printf("Failed to initialize score hook\n");
        return;
    }

    gf2DDrawScreen = reinterpret_cast<CGf2DDrawScreen>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gf2DDrawScreen@@YAXHHHH@Z"));
    gfFontSelectFontType = reinterpret_cast<CGfFontSelectFontType>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontSelectFontType@@YAXH@Z"));
    gfFontSelectEncodingType = reinterpret_cast<CGfFontSelectEncodingType>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontSelectEncodingType@@YAXH@Z"));
    gfFontOrigin = reinterpret_cast<CGfFontOrigin>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontOrigin@@YAXH@Z"));
    gfFontScale = reinterpret_cast<CGfFontScale>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontScale@@YAXMM@Z"));
    gfFontRotation = reinterpret_cast<CGfFontRotation>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontRotation@@YAXM@Z"));
    gfFontPosition = reinterpret_cast<CGfFontPosition>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontPosition@@YAXMM@Z"));
    gfFontColor3ub = reinterpret_cast<CGfFontColor3ub>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontColor3ub@@YAXEEE@Z"));
    gfFontShadow = reinterpret_cast<CGfFontShadow>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontShadow@@YAXH@Z"));
    gfFontGetPosition = reinterpret_cast<CGfFontGetPosition>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontGetPosition@@YAXPEAM0@Z"));
    gfFontPuts = reinterpret_cast<CGfFontPuts>(
        ::GetProcAddress(LoadLibraryA("libgdme.dll"), "?gfFontPuts@@YAXPEBX@Z"));
    sys_window_get_width = reinterpret_cast<CSysWindowGetWidth>(
        ::GetProcAddress(LoadLibraryA("libsystem.dll"), "?sys_window_get_width@@YAHXZ"));
    sys_window_get_height = reinterpret_cast<CSysWindowGetHeight>(
        ::GetProcAddress(LoadLibraryA("libsystem.dll"), "?sys_window_get_height@@YAHXZ"));
    sys_setting_get_param = reinterpret_cast<CSysSettingGetParam>(
        ::GetProcAddress(LoadLibraryA("libsystem.dll"), "sys_setting_get_param"));
    sys_code_get_software_specification_code =
        reinterpret_cast<CSysCodeGetSoftwareSpecificationCode>(::GetProcAddress(
            LoadLibraryA("libsystem.dll"), "sys_code_get_software_specification_code"));

    MH_CreateHook((char *)playerTimingDrumHookAddress, (void *)&timing_stats_drum_hook,
                  (void **)&real_timing_stats_drum_hook);
    MH_CreateHook((char *)playerTimingGuitarHookAddress, (void *)&timing_stats_guitar_hook,
                  (void **)&real_timing_stats_guitar_hook);

    MH_CreateHookApi(L"libshare-pj.dll",
                     "?mo_manager_set_music_play_info@@YAXPEBUmo_event_music_play_info_t@@@Z",
                     (void *)&my_mo_manager_set_music_play_info,
                     (void **)&real_mo_manager_set_music_play_info);
    MH_CreateHookApi(
        L"libshare-pj.dll",
        "?mo_manager_sync_set_music_select_info@@YAXPEBUmo_event_sync_music_select_info_t@@H@Z",
        (void *)&my_mo_manager_sync_set_music_select_info,
        (void **)&real_mo_manager_sync_set_music_select_info);
    MH_CreateHookApi(L"libshare-pj.dll", "?mo_manager_end@@YAHXZ", (void *)&my_mo_manager_end,
                     (void **)&real_mo_manager_end);
    MH_CreateHookApi(L"libcardunit.dll", "?cardunit_key_get@@YAHH@Z", (void *)&my_cardunit_key_get,
                     (void **)&real_cardunit_key_get);
    MH_CreateHookApi(L"libcardunit.dll", "?cardunit_card_read2@@YAHHQEAEPEAH@Z",
                     (void *)&my_cardunit_card_read2, (void **)&real_cardunit_card_read2);

    if (display_timing_stats) {
        MH_CreateHookApi(L"libshare-pj.dll", "?disp_paseli_step@@YAXXZ", (void *)&disp_paseli_step,
                         nullptr);
    }

    if (scorecard_hook) {
        ea3_xrpc_module_register(xrpcEndpointName, servicesUrlEndpoint, 0,
                                 fakeeascore_xrpc_methods);
    }

    MH_EnableHook(MH_ALL_HOOKS);
}

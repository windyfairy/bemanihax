#ifndef IMPORTS_AVS_H
#define IMPORTS_AVS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <windows.h>

#if !defined(AVS_VERSION)

#error "Can't build AVS-dependent project using AVS-independent make rules"

#elif AVS_VERSION == 1508 || AVS_VERSION == 1509

#define property_create XCd229cc000126
#define property_insert_read XCd229cc00009a
#define property_read_query_memsize XCd229cc0000ff
#define property_psmap_import XCd229cc000005
#define property_node_create XCd229cc00002c
#define avs_thread_delay XCd229cc00012b

#elif AVS_VERSION == 1700

#define property_create XCgsqzn0000090
#define property_insert_read XCgsqzn0000094
#define property_read_query_memsize XCgsqzn00000b0
#define property_psmap_import XCgsqzn00000b2

#else

#error AVS obfuscated import macros have not been declared for this version

#endif

enum property_type {
    PROPERTY_TYPE_VOID = 1,
    PROPERTY_TYPE_S8 = 2,
    PROPERTY_TYPE_U8 = 3,
    PROPERTY_TYPE_S16 = 4,
    PROPERTY_TYPE_U16 = 5,
    PROPERTY_TYPE_S32 = 6,
    PROPERTY_TYPE_U32 = 7,
    PROPERTY_TYPE_S64 = 8,
    PROPERTY_TYPE_U64 = 9,
    PROPERTY_TYPE_BIN = 10,
    PROPERTY_TYPE_STR = 11,
    PROPERTY_TYPE_FLOAT = 14,
    PROPERTY_TYPE_ATTR = 46,
    PROPERTY_TYPE_BOOL = 52,
};

struct property;
struct property_node;
struct property_psmap;

typedef int (*avs_reader_t)(uint32_t context, void *bytes, size_t nbytes);

uint32_t property_read_query_memsize(avs_reader_t reader, uint32_t context, int *nodes, int *total);
struct property *property_create(int flags, void *buffer, uint32_t buffer_size);
int property_insert_read(struct property *prop, struct property_node *node, avs_reader_t reader,
                         uint32_t context);
int property_psmap_import(struct property *prop, struct property_node *root, void *dest,
                          const struct property_psmap *psmap);
struct property_node *property_node_create(struct property *prop, struct property_node *parent,
                                           int type, const char *key, ...);

void avs_thread_delay(size_t ms, int zero);

#ifdef __cplusplus
}
#endif

#endif

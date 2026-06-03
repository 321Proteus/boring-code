#pragma once

#include "dr_api.h"
#include "dr_tools.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <zlib.h>
#include "counter.h"

#if defined(__x86_64__) || defined(_M_X64)
static const bool x64 = true;
#else
static const bool x64 = false;
#endif

#pragma pack(push, 1)
typedef struct {
    char magic[4];
    uint8_t version;
    uint8_t arch;
    uint32_t hash;
    uint64_t base;
} Header;
#pragma pack(pop)

// #pragma pack(push, 1)
typedef struct {
    uint32_t offset;
    module_counter_t module_id;
    thread_counter_t thread_id;
} bc_block_trace_t;
// #pragma pack(pop)

typedef struct {
    app_pc start;
    app_pc end;
    uint16_t module_id;
    uint16_t path_size;
    char path[];
} bc_module_trace_t;

// #pragma pack(push, 1)
typedef struct {
    uint32_t sysnum;
    uint8_t arg_count;
    uintptr_t args[8];
    uintptr_t retval;
} bc_syscall_trace_t;
// #pragma pack(pop)

// #pragma pack(push, 1)
typedef struct {
    app_pc pc;
    uint32_t code;
    uintptr_t xax, xbx, xcx, xdx;
    uintptr_t xsi, xdi, xbp, xsp;
} bc_exception_trace_t;
// #pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    app_pc instr_pc;
    uintptr_t value;
    uint8_t reg_id;
} bc_value_trace_t;
#pragma pack(pop)

typedef struct {
    app_pc start;
    app_pc end;
    module_counter_t id;
} module_entry_t;

typedef enum {
    EV_BBL = 0x1,
    EV_MODULE = 0x2,
    EV_SYSCALL = 0x3,
    EV_EXPCETION = 0x4
} bc_trace_event_type_t;

typedef struct {
    uint8_t type;
} trace_event_t;

#define TRACE_CHUNK_SIZE 10000
typedef struct {
    bc_block_trace_t buffer[TRACE_CHUNK_SIZE];
    int buf_idx;
    module_entry_t* last_mod;
    thread_counter_t virtual_id;
} thread_data_t;

uint32_t compute_crc32(const char* path) {

    file_t f = dr_open_file(path, DR_FILE_READ);
    if (f == INVALID_FILE) return 0;

    #define BUF_SIZE 1024
    unsigned char *buf = (unsigned char *)dr_global_alloc(BUF_SIZE);
    ssize_t nread;
    uint32_t crc = 0;

    while ((nread = dr_read_file(f, buf, 1024)) > 0) {
        crc = crc32(crc, buf, (size_t)nread);
    }

    dr_global_free(buf, BUF_SIZE);
    dr_close_file(f);
    return crc;
}

Header create_header(const char* path) {
    Header hdr;
    hdr.magic[0] = 'B';
    hdr.magic[1] = 'C';
    hdr.magic[2] = 'C';
    hdr.magic[3] = 'T';
    hdr.arch = x64;
    hdr.hash = (path ? compute_crc32(path) : 0);
    hdr.version = 0x01;
    return hdr;
}

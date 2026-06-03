#pragma once

#include "dr_api.h"
#include "dr_tools.h"
#include <stdint.h>
#include <stdbool.h>
#include <zlib.h>
#include "counter.h"
#include "core/format.h"

#if defined(__x86_64__) || defined(_M_X64)
static const bool x64 = true;
#else
static const bool x64 = false;
#endif

typedef struct {
    app_pc start;
    app_pc end;
    module_counter_t id;
} module_entry_t;

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

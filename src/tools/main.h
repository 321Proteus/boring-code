#pragma once

#include "dr_api.h"
#include "dr_tools.h"
#include <stdint.h>
#include <stdbool.h>
#include <zlib.h>
#include "counter.h"
#include "core/format.h"
#include "drx.h"

#if defined(__x86_64__) || defined(_M_X64)
static const bool x64 = true;
#else
static const bool x64 = false;
#endif

extern reg_id_t         tls_raw_base;
extern uint32_t         tls_raw_offset;
extern drx_buf_t*       trace_buf;

typedef struct {
    app_pc start;
    app_pc end;
    module_counter_t id;
} module_entry_t;

#define TRACE_CHUNK_SIZE 10000
typedef struct {
    thread_counter_t virtual_id;
} thread_data_t;

static uint32_t compute_crc32(const char* path) {

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

static Header create_header(const char* path) {
    Header hdr;
    hdr.magic[0] = 'B';
    hdr.magic[1] = 'C';
    hdr.magic[2] = 'C';
    hdr.magic[3] = 'T';
    hdr.arch = x64;
    hdr.hash = (path ? compute_crc32(path) : 0);
    hdr.version = BC_TRACE_FORMAT_VERSION;
    hdr.chunk_size = TRACE_CHUNK_SIZE;
    hdr.reserved = 0xFFFFFFFF;
    return hdr;
}

static void write_event(void* drcontext, void* ptr, size_t size, bc_trace_event_header_t header);


static inline void set_tls_vid_shifted(reg_id_t tls_raw_base, uint32_t tls_raw_offset, thread_counter_t vid) {
    byte* tls = dr_get_dr_segment_base(tls_raw_base);
#ifdef X64
    *(uint64_t*)(tls + tls_raw_offset) = (uint64_t)vid << 48;
#else
    *(uint32_t*)(tls + tls_raw_offset) = (uint32_t)vid;
#endif
}

static inline thread_counter_t get_tls_vid(reg_id_t tls_raw_base, uint32_t tls_raw_offset) {
    byte* tls = dr_get_dr_segment_base(tls_raw_base);
#ifdef X64
    return (thread_counter_t)(*(uint64_t*)(tls + tls_raw_offset) >> 48);
#else
    return (thread_counter_t)(*(uint32_t*)(tls + tls_raw_offset));
#endif
}
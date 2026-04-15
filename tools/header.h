#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <zlib.h>

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

uint32_t compute_crc32(const char* path) {

    file_t f = dr_open_file(path, DR_FILE_READ);
    if (f == INVALID_FILE) return 0;

    unsigned char buf[1024];
    ssize_t nread;
    uint32_t crc = 0;

    while ((nread = dr_read_file(f, buf, 1024)) > 0) {
        crc = crc32(crc, buf, (size_t)nread);
    }

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

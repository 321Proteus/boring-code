#pragma once

// #include <cstdint>
// #include <fstream>
// #include <vector>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <zlib.h>

typedef struct {
    char magic[4];
    uint8_t version;
    uint8_t arch;
    uint32_t hash;
} Header;

uint32_t compute_crc32(const char* path) {

    FILE* fp = fopen(path, "rb");

    if (fp == NULL) return 0;

    char buf[4096];
    uint64_t read;

    uint32_t crc = crc32(0L, Z_NULL, 0);

    while ((read = fread(buf, 1, sizeof(buf), fp)) > 0) {
        crc = crc32(crc, (const Bytef*)buf, read);
    }

    return crc;
}

Header create_header(const char* path, bool is_x64) {
    Header hdr;
    hdr.magic[0] = 'B';
    hdr.magic[1] = 'C';
    hdr.magic[2] = 'C';
    hdr.magic[3] = 'T';
    hdr.arch = is_x64;
    hdr.hash = (path ? compute_crc32(path) : 0);
    hdr.version = 0x01;
    return hdr;
}

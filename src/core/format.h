#pragma once

#include <stdint.h>
#include "tools/counter.h"

#define BC_TRACE_FORMAT_VERSION 0x02;

#pragma pack(push, 1)
typedef struct {
    char magic[4];
    uint8_t version;
    uint8_t arch;
    uint32_t hash;
    uint32_t base_low;
    uint16_t base_mid;
} Header;
#pragma pack(pop)

// #pragma pack(push, 1)
typedef struct {
    uint32_t pc_low;
    uint16_t pc_mid;
    thread_counter_t thread_id;
} bc_block_trace_t;
// #pragma pack(pop)

typedef struct {
    uint64_t start;
    uint64_t end;
    uint16_t module_id;
    uint16_t path_size;
    char path[];
} bc_module_trace_t;

// #pragma pack(push, 1)
typedef struct {
    uint32_t sysnum;
    uint8_t arg_count;
    uint64_t args[8];
    uint64_t retval;
} bc_syscall_trace_t;
// #pragma pack(pop)

// #pragma pack(push, 1)
typedef struct {
    uint64_t pc;
    uint32_t code;
    thread_counter_t thread_id;
    uint64_t xax, xbx, xcx, xdx;
    uint64_t xsi, xdi, xbp, xsp;
} bc_exception_trace_t;
// #pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint64_t instr_pc;
    uint64_t value;
    uint8_t reg_id;
} bc_value_trace_t;
#pragma pack(pop)

typedef enum {
    EV_BBL,
    EV_MODULE,
    EV_SYSCALL,
    EV_EXCEPTION
} bc_trace_event_type_t;

typedef struct {
    uint8_t reserved;
    uint8_t type;
    uint16_t count;
} bc_trace_event_header_t;
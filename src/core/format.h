#pragma once

#include <stdint.h>
#include "tools/counter.h"

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
    EV_BBL = 0x1,
    EV_MODULE = 0x2,
    EV_SYSCALL = 0x3,
    EV_EXPCETION = 0x4
} bc_trace_event_type_t;

typedef struct {
    uint8_t type;
} bc_trace_event_header_t;
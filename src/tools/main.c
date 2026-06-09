#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dr_api.h"
#include "dr_defines.h"
#include "drx.h"
#include "drreg.h"
#include "dr_modules.h"
#include "dr_events.h"
#include "dr_tools.h"
#include "drmgr.h"
#include "main.h"
#include "core/format.h"

static int              module_count = 0;
static volatile int     thread_count = 0;

static int              tls_index;
static reg_id_t         tls_raw_base;
static unsigned int     tls_raw_offset;

static drx_buf_t*       trace_buf;

static file_t           out_file;
static file_t           log_file;

static app_pc           main_mod_start = NULL;
static app_pc           main_mod_end = NULL;

static volatile int     trace_size = 0;
static int              max_trace_size = 0; 
static volatile bool    limit_reached = false;

static bool             no_syscalls = false;

static void flush_buf_part(void* drcontext) {

    void* base = drx_buf_get_buffer_base(drcontext, trace_buf);
    void* ptr = drx_buf_get_buffer_ptr(drcontext, trace_buf);
    size_t remaining = (byte*)ptr - (byte*)base;
    dr_printf("Flushing %d buffer entries\n", remaining);
    if (remaining > 0) dr_write_file(out_file, base, remaining);

}

static void on_buf_full(void* drcontext, void* buf_base, size_t size) {
    dr_write_file(out_file, buf_base, size);
    if (max_trace_size > 0 &&
        (uint32_t)dr_atomic_add32_return_sum(&trace_size,
            (int)(size / sizeof(bc_block_trace_t))) >= (uint32_t)max_trace_size) {
        limit_reached = true;
        dr_fprintf(STDERR, "Limit reached, stopping trace\n");
        dr_exit_process(0);
    }
}

static void event_thread_init(void* drcontext) {

    thread_data_t* data = dr_thread_alloc(drcontext, sizeof(thread_data_t));
    data->last_mod = NULL;
    data->virtual_id = dr_atomic_add32_return_sum(&thread_count, 1) - 1;
    
    drmgr_set_tls_field(drcontext, tls_index, data);
    
    char buf[32];
    int l = dr_snprintf(buf, sizeof(buf), "Thread %d (vid %d) init\n", dr_get_thread_id(drcontext), data->virtual_id);
    dr_fprintf(STDERR, "Thread %d (vid %d) init\n", dr_get_thread_id(drcontext), data->virtual_id);
    dr_write_file(log_file, buf, l);
    
    byte *tls_base = dr_get_dr_segment_base(tls_raw_base);
    *(reg_t*)(tls_base + tls_raw_offset) = (reg_t)data->virtual_id;

}

static void event_thread_exit(void* drcontext) {

    flush_buf_part(drcontext);

    thread_data_t* data = drmgr_get_tls_field(drcontext, tls_index);    
    char buf[32];
    int l = dr_snprintf(buf, sizeof(buf), "Thread %d (vid %d) exit\n", dr_get_thread_id(drcontext), data->virtual_id);
    dr_fprintf(STDERR, "Thread %d (vid %d) exit\n", dr_get_thread_id(drcontext), data->virtual_id);
    
    dr_write_file(log_file, buf, l);
    dr_flush_file(log_file);

    byte *tls_base = dr_get_dr_segment_base(tls_raw_base);
    *(reg_t*)(tls_base + tls_raw_offset) = 0;

    dr_thread_free(drcontext, data, sizeof(thread_data_t));
    
}

static void event_module_load(void *drcontext, const module_data_t *info, char loaded) {

    char buf[256];

    const char* name = dr_module_preferred_name(info);
    const char* path = info->full_path;
    size_t path_size = strlen(path) + 1;

    bc_module_trace_t* mod = (bc_module_trace_t*)dr_global_alloc(sizeof(bc_module_trace_t) + path_size);

    mod->module_id = module_count++;
    mod->start = (uint64_t)info->start;
    mod->end = (uint64_t)info->end;
    mod->path_size = (uint16_t)path_size;
    strncpy(mod->path, path, path_size);

    int l = dr_snprintf(buf, sizeof(buf),
    "Loaded module %s (#%d)\n\tStart: %llX\n\tEnd: %llX\n\tEP: %llX\n\tPath: %s\n\n",
        name, mod->module_id, mod->start, mod->end, info->entry_point, mod->path
    );

    dr_global_free(mod, sizeof(bc_module_trace_t) + path_size);
    dr_write_file(log_file, buf, l);

}

static void record_bbl(app_pc pc) {

    void* drcontext = dr_get_current_drcontext();
    thread_data_t* data = drmgr_get_tls_field(drcontext, tls_index);
    if (data == NULL) return;

    bc_block_trace_t tr;
    tr.pc_low = (uint32_t)(uintptr_t)pc;
    tr.pc_mid = (uint16_t)((uintptr_t)pc >> 32);
    tr.thread_id = data->virtual_id;

    void* ptr = drx_buf_get_buffer_ptr(drcontext, trace_buf);
    void *base = drx_buf_get_buffer_base(drcontext, trace_buf);
    size_t capacity = drx_buf_get_buffer_size(drcontext, trace_buf);

    if ((byte *)ptr + sizeof(bc_block_trace_t) > (byte *)base + capacity) {
        on_buf_full(drcontext, base, (byte *)ptr - (byte *)base);
        ptr = drx_buf_get_buffer_ptr(drcontext, trace_buf);
    }

    *(bc_block_trace_t*)ptr = tr;
    drx_buf_set_buffer_ptr(drcontext, trace_buf,
        (byte*)ptr + sizeof(bc_block_trace_t)
    );

}

#ifdef _WIN32
    #define DR_TLS_SEG DR_SEG_GS
#else
    #define DR_TLS_SEG DR_SEG_FS
#endif

static void emit_inline_record(void* drcontext, instrlist_t* bb, instr_t* first, app_pc pc) {

    reg_id_t reg1, reg2;

    if (drreg_reserve_register(drcontext, bb, first, NULL, &reg1) != DRREG_SUCCESS ||
        drreg_reserve_register(drcontext, bb, first, NULL, &reg2) != DRREG_SUCCESS) {
        dr_insert_clean_call(drcontext, bb, first, (void*)record_bbl,
            false, 1, OPND_CREATE_INTPTR(pc));
        return;
    }
    
    uint32_t pc_low = (uint32_t)(uintptr_t)pc;
    uint16_t pc_mid = (uint16_t)((uintptr_t)pc >> 32);

    dr_insert_read_raw_tls(drcontext, bb, first,
        tls_raw_base, tls_raw_offset, reg2
    );

    drx_buf_insert_load_buf_ptr(drcontext, trace_buf, bb, first, reg1);

    drx_buf_insert_buf_store(drcontext, trace_buf, bb, first,
        reg1, DR_REG_NULL,
        OPND_CREATE_INT32((int32_t)pc_low), OPSZ_4,
        offsetof(bc_block_trace_t, pc_low)
    );

    drx_buf_insert_buf_store(
        drcontext, trace_buf, bb, first, 
        reg1, DR_REG_NULL,
        OPND_CREATE_INT16(pc_mid), OPSZ_2,
        offsetof(bc_block_trace_t, pc_mid)
    );

    drx_buf_insert_buf_store(drcontext, trace_buf, bb, first,
        reg1, DR_REG_NULL,
        opnd_create_reg(reg_resize_to_opsz(reg2, OPSZ_2)), OPSZ_2,
        offsetof(bc_block_trace_t, thread_id)
    );

    drx_buf_insert_update_buf_ptr(drcontext, trace_buf, bb, first,
        reg1, DR_REG_NULL, sizeof(bc_block_trace_t)
    );

    drreg_unreserve_register(drcontext, bb, first, reg1);
    drreg_unreserve_register(drcontext, bb, first, reg2);

}

dr_emit_flags_t event_basic_block(void *drcontext, void *tag,
    instrlist_t *bb, instr_t *instr, char for_trace, char translating, void *user_data) {

    if (!drmgr_is_first_instr(drcontext, instr)) return DR_EMIT_DEFAULT;

    instr_t* first = instrlist_first_app(bb);
    app_pc pc = instr_get_app_pc(first);

    if (limit_reached) return DR_EMIT_DEFAULT;

    if (no_syscalls && (pc < main_mod_start || pc >= main_mod_end)) return DR_EMIT_DEFAULT;
    emit_inline_record(drcontext, bb, first, pc);

    return DR_EMIT_DEFAULT;
}

static void print_exception(thread_data_t* data, bc_exception_trace_t e) {

    dr_fprintf(STDERR, "Exception");
    char buf[256];
    int l = dr_snprintf(buf, sizeof(buf), 
        "Exception %X at %X\n"
        "EAX: %X EBX: %X ECX: %X EDX: %X\n"
        "ESI: %X EDI: %X EBP: %X ESP: %X\n",
        e.code, e.pc,
        e.xax, e.xbx, e.xcx, e.xdx,
        e.xsi, e.xdi, e.xbp, e.xsp);
        
    dr_write_file(log_file, buf, l);
    
    flush_buf_part(dr_get_current_drcontext());

}

#ifdef _WIN32
static char event_exception(void* drcontext, dr_exception_t* except)
{
    thread_data_t* data = drmgr_get_tls_field(drcontext, tls_index);
    dr_mcontext_t* mc = except->mcontext;

    bc_exception_trace_t e = {
        (uint64_t)mc->pc,
        except->record->ExceptionCode,
        mc->xax, mc->xbx, mc->xcx, mc->xdx,
        mc->xsi, mc->xdi, mc->xbp, mc->xsp
    };

    print_exception(data, e);

    return true;
}
#else
static dr_signal_action_t event_signal(void* drcontext, dr_siginfo_t* info) {

    thread_data_t* data = drmgr_get_tls_field(drcontext, tls_index);
    dr_mcontext_t* mc = info->mcontext;

    bc_exception_trace_t e = {
        info->sig,
        (uint64_t)mc->pc,
        mc->xax, mc->xbx, mc->xcx, mc->xdx,
        mc->xsi, mc->xdi, mc->xbp, mc->xsp
    };

    print_exception(data, e);

    return DR_SIGNAL_DELIVER;
}
#endif

static void event_exit(void) {

    dr_write_file(log_file, "App closed\n", 10);
    dr_flush_file(log_file);

    dr_close_file(log_file);
    dr_close_file(out_file);

    drmgr_unregister_tls_field(tls_index);
    dr_raw_tls_cfree(tls_raw_offset, 1);
    drx_buf_free(trace_buf);

    drx_exit();
    drreg_exit();
    drmgr_exit();
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char* argv[])
{

#ifdef _WIN32
    dr_enable_console_printing();
#endif

    dr_fprintf(STDERR, "buffer offset:    %zu\n", offsetof(thread_data_t, buffer));
    dr_fprintf(STDERR, "virtual_id offset:%zu\n", offsetof(thread_data_t, virtual_id));

    bool custom_name = false;
    char fname[64];
    Header hdr;

    for (int i=0;i<argc;i++) {
        if (strcmp(argv[i], "-o") == 0 && i != argc - 1) {
            custom_name = true;
            sprintf(fname, "%s", argv[i+1]);
        }
        if (strcmp(argv[i], "-n") == 0 && i != argc - 1) {
            max_trace_size = strtol(argv[i+1], NULL, 10);
        }
        if (strcmp(argv[i], "--no-syscalls") == 0) {
            no_syscalls = true;
        }
    }

    if (!drmgr_init()) DR_ASSERT_MSG(false, "drmgr_init() failed");
    if (!drx_init()) DR_ASSERT_MSG(false, "drx_init() failed");

    drreg_options_t drreg_ops = {sizeof(drreg_ops), 2, false};
    if (drreg_init(&drreg_ops) != DRREG_SUCCESS) DR_ASSERT_MSG(false, "drreg_init() failed");

    if (!dr_raw_tls_calloc(&tls_raw_base, &tls_raw_offset, 1, 0))
        DR_ASSERT_MSG(false, "Failed to allocate TLS buffer");
    dr_fprintf(STDERR, "tls_raw_base=%d tls_raw_offset=%u\n",
        (int)tls_raw_base, tls_raw_offset
    );

    trace_buf = drx_buf_create_trace_buffer(
        TRACE_CHUNK_SIZE * sizeof(bc_block_trace_t), on_buf_full
    );
    DR_ASSERT_MSG(trace_buf != NULL, "Failed to create the trace buffer");
    
    tls_index = drmgr_register_tls_field();

    dr_set_client_name("BoringTool Tracer", "http://dynamorio.org/");
    log_file = dr_open_file("boring_log.txt", DR_FILE_WRITE_OVERWRITE);
    module_data_t* main_mod = dr_get_main_module();

    if (main_mod) {

        const char* path = main_mod->full_path; 

        char buf[256];
        int l = dr_snprintf(buf, sizeof(buf),
        "Starting analysis of target \"%s\"\nArgs: max_trace_size=%llu, no_syscalls=%s\n", path, max_trace_size, (no_syscalls ? "true" : "false"));
        dr_write_file(log_file, buf, l);

        hdr = create_header(path);
        hdr.base = (uint64_t)(uintptr_t)main_mod->start;

        main_mod_start = main_mod->start;
        main_mod_end = main_mod->end;

        if (!custom_name) sprintf(fname, "bcfunctions_%X.bin", hdr.hash);
        dr_free_module_data(main_mod);

    } else {

        hdr = create_header(NULL);
        hdr.base = 0;

        const char* warning = "Warning: Couldn't find the main module, which probably indicates a broken or obfuscated binary. \
            Image base and CRC32 have been set to their default values.";
        dr_write_file(log_file, warning, strlen(warning));

        if (!custom_name) sprintf(fname, "bcfunctions.bin");

    }

    out_file = dr_open_file(fname, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);

    dr_write_file(out_file, &hdr, sizeof(Header));

    drmgr_register_thread_init_event(event_thread_init);
    drmgr_register_thread_exit_event(event_thread_exit);
    drmgr_register_module_load_event(event_module_load);

    #ifdef _WIN32
        drmgr_register_exception_event(event_exception);
    #else
        drmgr_register_signal_event(event_signal);
    #endif

    drmgr_register_bb_instrumentation_event(
        NULL,
        event_basic_block,
        NULL
    );
    dr_register_exit_event(event_exit);
}
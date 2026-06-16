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
#include "record.h"

static module_counter_t module_count = 0;
static thread_counter_t thread_count = 0;

reg_id_t                tls_raw_base;
uint32_t                tls_raw_offset;

drx_buf_t*              trace_buf;

static void*            write_lock;
static file_t           out_file;
static file_t           log_file;

static app_pc           main_mod_start = NULL;
static app_pc           main_mod_end = NULL;

static volatile int     trace_size = 0;
static int              max_trace_size = 0; 
static volatile int     limit_reached = false;
static volatile int     thread_exit = false;

static bool             no_syscalls = false;

static void flush_buf_part(void* drcontext) {

    void* base = drx_buf_get_buffer_base(drcontext, trace_buf);
    void* ptr = drx_buf_get_buffer_ptr(drcontext, trace_buf);
    size_t remaining = (byte*)ptr - (byte*)base;
    size_t rem_count = (int)(remaining / sizeof(bc_block_trace_t));
    dr_printf("Flushing %d (%td) buffer entries\n", rem_count, remaining);

    if (remaining > 0) {
        bc_trace_event_header_t part_header = { 0xFF, EV_BBL, rem_count };
        write_event(drcontext, base, remaining, part_header);
    }
}

static void write_event(void* drcontext, void* ptr, size_t size, bc_trace_event_header_t header) {
    if (header.type != EV_BBL) flush_buf_part(drcontext);

    dr_mutex_lock(write_lock);
    dr_write_file(out_file, &header, sizeof(bc_trace_event_header_t));
    dr_write_file(out_file, ptr, size);
    dr_mutex_unlock(write_lock);
    
}

static void on_buf_full(void* drcontext, void* buf_base, size_t size) {

    int count = (int)(size / sizeof(bc_block_trace_t));

    bc_trace_event_header_t header = { 0xFF, EV_BBL, count };
    write_event(drcontext, buf_base, size, header);

    if (max_trace_size > 0 && dr_atomic_add32_return_sum(&trace_size, count) >= max_trace_size) {
        if (dr_atomic_add32_return_sum(&thread_exit, 1) == 1) {
            dr_atomic_store32(&limit_reached, 1);
            dr_fprintf(STDERR, "Limit reached, stopping trace\n");
        }
    }
}
// TODO: Fix close
static void exit_app(void) {
    // if (dr_atomic_add32_return_sum(&thread_exit, 1) != 1) return;
    dr_abort();
    // dr_exit_process(0);
}

static void event_thread_init(void* drcontext) {

    thread_counter_t vid = (thread_counter_t)(dr_atomic_add32_return_sum(
        (volatile int *)&thread_count, 1) - 1
    );

    set_tls_vid_shifted(tls_raw_base, tls_raw_offset, vid);
    
    char buf[32];
    int l = dr_snprintf(buf, sizeof(buf), "Thread %d (vid %d) init\n", dr_get_thread_id(drcontext), vid);
    dr_write_file(log_file, buf, l);
    dr_fprintf(STDERR, "Thread %d (vid %d) init\n", dr_get_thread_id(drcontext), vid);

}

static void event_thread_exit(void* drcontext) {

    flush_buf_part(drcontext);

    thread_counter_t vid = get_tls_vid(tls_raw_base, tls_raw_offset);

    char buf[32];
    int l = dr_snprintf(buf, sizeof(buf), "Thread %d (vid %d) exit\n", dr_get_thread_id(drcontext), vid);
    dr_fprintf(STDERR, "Thread %d (vid %d) exit\n", dr_get_thread_id(drcontext), vid);
    
    dr_write_file(log_file, buf, l);
    dr_flush_file(log_file);
    
}

static void event_module_load(void *drcontext, const module_data_t *info, char loaded) {

    char buf[512];

    const char* name = dr_module_preferred_name(info);
    const char* path = info->full_path;
    size_t path_size = strlen(path);
    size_t total_size = sizeof(bc_module_trace_t) + path_size;

    bc_module_trace_t* mod = (bc_module_trace_t*)dr_global_alloc(total_size);

    mod->module_id = module_count++;
    mod->start = (uintptr_t)info->start;
    mod->end = (uintptr_t)info->end;
    mod->path_size = (uint16_t)path_size;
    memcpy(mod->path, path, path_size);

    int l = dr_snprintf(buf, sizeof(buf),
    "Loaded module %s (#%d)\n\tStart: %llX\n\tEnd: %llX\n\tEP: %p\n\tPath: %s\n\n",
        name, mod->module_id, mod->start, mod->end, info->entry_point, mod->path
    );

    bc_trace_event_header_t header = { 0xFF, EV_MODULE, 1 };
    write_event(drcontext, mod, total_size, header);

    dr_global_free(mod, sizeof(bc_module_trace_t) + path_size);
    dr_write_file(log_file, buf, l);

}

static void record_bbl(app_pc pc) {

    void* drcontext = dr_get_current_drcontext();

    bc_block_trace_t tr;
    tr.pc_low = (uint32_t)(uintptr_t)pc;
#ifdef X86_64
    tr.pc_mid = (uint16_t)((uintptr_t)pc >> 32);
#endif

    tr.thread_id = get_tls_vid(tls_raw_base, tls_raw_offset);

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

dr_emit_flags_t event_basic_block(void *drcontext, void *tag,
    instrlist_t *bb, instr_t *instr, char for_trace, char translating, void *user_data) {

    if (!drmgr_is_first_instr(drcontext, instr)) return DR_EMIT_DEFAULT;

    instr_t* first = instrlist_first_app(bb);
    app_pc pc = instr_get_app_pc(first);

    if (dr_atomic_load32(&limit_reached)) {
        dr_insert_clean_call(drcontext, bb, first,
            (void*)exit_app, false,  0, false);
        return DR_EMIT_DEFAULT;
    }

    // if (no_syscalls && (pc < main_mod_start || pc >= main_mod_end)) return DR_EMIT_DEFAULT;
    emit_inline_record(drcontext, bb, first, pc);

    return DR_EMIT_DEFAULT;
}

static void save_exception(bc_exception_trace_t e, thread_id_t global_id) {

    dr_fprintf(STDERR, "Exception %X\n", e.code);
    char buf[256];
    int l = dr_snprintf(buf, sizeof(buf), 
        "Thread %d (vid %d) Exception %X at %X\n"
        "EAX: %X EBX: %X ECX: %X EDX: %X\n"
        "ESI: %X EDI: %X EBP: %X ESP: %X\n",
        global_id, e.thread_id, e.code, e.pc,
        e.xax, e.xbx, e.xcx, e.xdx,
        e.xsi, e.xdi, e.xbp, e.xsp);
        
    dr_write_file(log_file, buf, l);

    bc_trace_event_header_t header = { 0xFF, EV_EXCEPTION, 1 };
    write_event(dr_get_current_drcontext(), &e, sizeof(bc_exception_trace_t), header);

}

#ifdef _WIN32
static char event_exception(void* drcontext, dr_exception_t* except)
{
    thread_counter_t virtual_id = get_tls_vid(tls_raw_base, tls_raw_offset);
    thread_id_t full_id = dr_get_thread_id(drcontext);
    dr_mcontext_t* mc = except->mcontext;

    bc_exception_trace_t e = {
        (uint64_t)mc->pc,
        except->record->ExceptionCode,
        virtual_id,
        mc->xax, mc->xbx, mc->xcx, mc->xdx,
        mc->xsi, mc->xdi, mc->xbp, mc->xsp
    };

    save_exception(e, full_id);

    return true;
}
#else
static dr_signal_action_t event_signal(void* drcontext, dr_siginfo_t* info) {

    thread_counter_t virtual_id = get_tls_vid(tls_raw_base, tls_raw_offset);
    thread_id_t full_id = dr_get_thread_id(drcontext);
    dr_mcontext_t* mc = info->mcontext;

    bc_exception_trace_t e = {
        (uintptr_t)mc->pc,
        info->sig,
        virtual_id,
        mc->xax, mc->xbx, mc->xcx, mc->xdx,
        mc->xsi, mc->xdi, mc->xbp, mc->xsp
    };

    save_exception(e, full_id);

    return DR_SIGNAL_DELIVER;
}
#endif

static void event_exit(void) {

    dr_printf("Stopping\n");
    dr_write_file(log_file, "App closed\n", 10);
    dr_flush_file(log_file);

    dr_close_file(log_file);
    dr_close_file(out_file);

    dr_mutex_destroy(write_lock);

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

    drreg_options_t drreg_ops = { sizeof(drreg_ops), 2, false };
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

    write_lock = dr_mutex_create();

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

        main_mod_start = main_mod->start;
        main_mod_end = main_mod->end;

        if (!custom_name) sprintf(fname, "bctrace_%X.bin", hdr.hash);

    } else {

        hdr = create_header(NULL);

        const char* warning = "Warning: Couldn't find the main module, which probably indicates a broken or obfuscated binary. \
            The checksum has been set to the default value.";
        dr_write_file(log_file, warning, strlen(warning));

        if (!custom_name) sprintf(fname, "bctrace.bin");

    }

    dr_free_module_data(main_mod);

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
    drmgr_register_exit_event(event_exit);
}
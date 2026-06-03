#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dr_api.h"
#include "dr_defines.h"
#include "dr_modules.h"
#include "dr_events.h"
#include "dr_tools.h"
#include "drmgr.h"
#include "main.h"
#include "core/format.h"

static module_entry_t   modules[MAX_MODULES];
static uint16_t         module_count = 0;

static volatile int     thread_count = 0;

static int              tls_index;

static file_t           out_file;
static file_t           log_file;

static app_pc           main_mod_start = NULL;
static app_pc           main_mod_end = NULL;

static volatile int     trace_size = 0;
static int              max_trace_size = 0; 

static bool             no_syscalls = false;

static void             *module_lock;

static module_entry_t* get_module_id(void* drcontext, app_pc pc) {

    thread_data_t* data = (thread_data_t*)drmgr_get_tls_field(drcontext, tls_index);
    if (data->last_mod && pc >= data->last_mod->start && pc < data->last_mod->end) return data->last_mod;

    for (uint16_t i=0;i<module_count;i++) {
        if (pc >= modules[i].start && pc < modules[i].end) {
            data->last_mod = &modules[i];
            return data->last_mod;
        }
    }
    module_data_t* fallback = dr_lookup_module(pc);
    if (fallback) {
        module_entry_t found;

        dr_mutex_lock(module_lock);
        found.id = module_count;
        found.start = fallback->start;
        found.end = fallback->end;

        dr_free_module_data(fallback);
        
        modules[module_count] = found;
        module_entry_t* result = &modules[module_count];
        module_count++;

        dr_mutex_unlock(module_lock);

        return result;

    } else {
        return NULL;
    }

}

void flush_buffer(thread_data_t* data) {
    dr_write_file(out_file, data->buffer, data->buf_idx * sizeof(bc_block_trace_t));
    data->buf_idx = 0;
}

static void event_thread_init(void* drcontext) {

    thread_data_t *data = dr_thread_alloc(drcontext, sizeof(thread_data_t));
    data->buf_idx = 0;
    data->last_mod = NULL;
    data->virtual_id = dr_atomic_add32_return_sum(&thread_count, 1) - 1;
    drmgr_set_tls_field(drcontext, tls_index, data);
    
    char buf[32];
    int l = dr_snprintf(buf, sizeof(buf), "Thread %d (vid %d) init\n", dr_get_thread_id(drcontext), data->virtual_id);
    dr_write_file(log_file, buf, l);

}

static void event_thread_exit(void* drcontext) {

    thread_data_t *data = drmgr_get_tls_field(drcontext, tls_index);
    if (data->buf_idx > 0) flush_buffer(data);
    
    char buf[32];
    int l = dr_snprintf(buf, sizeof(buf), "Thread %d (vid %d) exit\n", dr_get_thread_id(drcontext), data->virtual_id);
    dr_write_file(log_file, buf, l);
    dr_flush_file(log_file);

    dr_thread_free(drcontext, data, sizeof(thread_data_t));
    
}

static void event_module_load(void *drcontext, const module_data_t *info, char loaded) {

    char buf[256];

    const char* name = dr_module_preferred_name(info);

    const char* path = info->full_path;
    size_t path_size = strlen(path);

    bc_module_trace_t* mod = (bc_module_trace_t*)dr_global_alloc(sizeof(bc_module_trace_t) + path_size + 1);

    mod->module_id = module_count;
    mod->start = (uint64_t)info->start;
    mod->end = (uint64_t)info->end;
    mod->path_size = (uint16_t)path_size;
    strncpy(mod->path, path, path_size);
    mod->path[path_size] = '\0';

    module_entry_t entry;
    entry.id = module_count;
    entry.start = info->start;
    entry.end = info->end;

    dr_mutex_lock(module_lock);
    modules[module_count++] = entry;
    dr_mutex_unlock(module_lock);

    int l = dr_snprintf(buf, sizeof(buf),
    "Loaded module %s (#%d)\n\tStart: %llX\n\tEnd: %llX\n\tEP: %llX\n\tPath: %s\n\n",
        name, mod->module_id, mod->start, mod->end, info->entry_point, mod->path
    );

    dr_global_free(mod, sizeof(bc_module_trace_t) + path_size + 1);
    dr_write_file(log_file, buf, l);

}

static char fuse = false;
static volatile char limit_reached = false; 

static void record_bbl(void* drcontext, app_pc pc)
{
    thread_data_t* data = drmgr_get_tls_field(drcontext, tls_index);
    module_entry_t* module = get_module_id(drcontext, pc);

    if (!module) {
        char t[64];
        int l = dr_snprintf(t, sizeof(t), "Null instr %d pc %llX\n", data->buf_idx, pc);
        dr_write_file(log_file, t, l);
        return;
    };

    bc_block_trace_t tr;
    tr.thread_id = data->virtual_id;
    tr.module_id = module->id;
    tr.offset = pc - module->start;

    data->buffer[data->buf_idx++] = tr;
    if (data->buf_idx >= TRACE_CHUNK_SIZE) flush_buffer(data);

    if (max_trace_size > 0 && (uint64_t)dr_atomic_add32_return_sum(&trace_size, 1) >= max_trace_size) {
        limit_reached = true;
        flush_buffer(data);
        dr_exit_process(0);
    }

}

dr_emit_flags_t event_basic_block(void *drcontext, void *tag,
    instrlist_t *bb, instr_t *instr, char for_trace, char translating, void *user_data)
{
    if(!drmgr_is_first_instr(drcontext, instr)) return DR_EMIT_DEFAULT;

    instr_t* first = instrlist_first_app(bb);
    app_pc pc = instr_get_app_pc(first);

    if (no_syscalls && (pc < main_mod_start || pc >= main_mod_end)) return DR_EMIT_DEFAULT;

    dr_insert_clean_call(drcontext, bb, first, (void*)record_bbl,
        false, 2, OPND_CREATE_INTPTR(drcontext), OPND_CREATE_INTPTR(pc));

    return DR_EMIT_DEFAULT;
}

static void print_exception(thread_data_t* data, bc_exception_trace_t e) {

    char buf[256];
    int l = dr_snprintf(buf, sizeof(buf), 
        "Exception %X at %X\n"
        "EAX: %X EBX: %X ECX: %X EDX: %X\n"
        "ESI: %X EDI: %X EBP: %X ESP: %X\n",
        e.code, e.pc,
        e.xax, e.xbx, e.xcx, e.xdx,
        e.xsi, e.xdi, e.xbp, e.xsp);
        
    dr_write_file(log_file, buf, l);
    
    if (data->buf_idx > 0) flush_buffer(data);

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
    return DR_SIGNAL_DELIVER;
}
#endif

static void event_exit(void)
{
    dr_write_file(log_file, "App closed\n", 10);
    dr_flush_file(log_file);
    dr_mutex_destroy(module_lock);
    drmgr_unregister_tls_field(tls_index);
    drmgr_exit();
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char* argv[])
{

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

    drmgr_init();
    dr_set_client_name("BoringTool Tracer", "http://dynamorio.org/");

    log_file = dr_open_file("boring_log.txt", DR_FILE_WRITE_OVERWRITE);
    
    tls_index = drmgr_register_tls_field();

    module_data_t *main_mod = dr_get_main_module();

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

    module_lock = dr_mutex_create();

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
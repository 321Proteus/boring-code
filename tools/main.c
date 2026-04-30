#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dr_api.h"
#include "dr_defines.h"
#include "dr_modules.h"
#include "dr_events.h"
#include "dr_tools.h"
#include "drmgr.h"
#include "header.h"

#define CHUNK_SIZE 10000

typedef struct {
    app_pc buffer[CHUNK_SIZE];
    int buf_idx;
    file_t file;
} thread_data_t;

static int tls_index;

static file_t out_file;

static app_pc main_mod_start = NULL;
static app_pc main_mod_end = NULL;

static const char* app_name;

static file_t log_file;
static bool no_syscalls = false;

typedef struct {
    app_pc start;
    app_pc end;
    char name[MAX_PATH_LENGTH];
} module_range_t;

#define MAX_SYS_MODULES 64
static module_range_t sys_modules[MAX_SYS_MODULES];
static int sys_mod_count = 0;

static void event_thread_init(void* drcontext) {

    char buf[32];
    int l = dr_snprintf(buf, sizeof(buf), "Thread %d init\n", dr_get_thread_id(drcontext));
    dr_write_file(log_file, buf, l);

    thread_data_t *data = dr_thread_alloc(drcontext, sizeof(thread_data_t));
    data->buf_idx = 0;
    data->file = out_file;

    drmgr_set_tls_field(drcontext, tls_index, data);

}

static void event_thread_exit(void* drcontext) {

    char buf[32];
    int l = dr_snprintf(buf, sizeof(buf), "Thread %d exit\n", dr_get_thread_id(drcontext));
    dr_write_file(log_file, buf, l);
    dr_flush_file(log_file);

    thread_data_t *data = drmgr_get_tls_field(drcontext, tls_index);

    if (data->buf_idx > 0) {
        dr_write_file(data->file, data->buffer,
                      data->buf_idx * sizeof(app_pc));
    }

    dr_thread_free(drcontext, data, sizeof(thread_data_t));
    
}

static void event_module_load(void *drcontext, const module_data_t *info, bool loaded) {
    char buf[256];
    int l = dr_snprintf(
        buf,
        sizeof(buf),
        "Loaded module %s\n\tStart:%X\n\tEnd:%X\n\tEP:%X\n",
        info->full_path, info->start, info->end, info->entry_point
    );
    dr_write_file(log_file, buf, l);
    if (sys_mod_count < MAX_SYS_MODULES) {
            sys_modules[sys_mod_count].start = info->start;
            sys_modules[sys_mod_count].end = info->end;
            dr_snprintf(sys_modules[sys_mod_count].name, MAX_PATH_LENGTH, "%s", dr_module_preferred_name(info));
            sys_mod_count++;
        }
}

static bool is_pc_in_system_module(app_pc pc) {
    for (int i = 0; i < sys_mod_count; i++) {
        if (pc >= sys_modules[i].start && pc < sys_modules[i].end) return true;
    }
    return false;
}

static void flush_buffer(thread_data_t* data)
{
    if (data->file != INVALID_FILE)
    {
        dr_write_file(data->file, data->buffer, data->buf_idx * sizeof(app_pc));
        data->buf_idx = 0;
    }
}

static void record_bbl(void* drcontext, app_pc pc)
{
    thread_data_t *data = drmgr_get_tls_field(drcontext, tls_index);
    data->buffer[data->buf_idx++] = pc;
    if (data->buf_idx >= CHUNK_SIZE) flush_buffer(data);
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

static void print_exception(thread_data_t* data, bc_exception_trace e) {

    char buf[256];
    int l = dr_snprintf(buf, sizeof(buf), 
        "Exception %X at %X\n"
        "EAX: %X EBX: %X ECX: %X EDX: %X\n"
        "ESI: %X EDI: %X EBP: %X ESP: %X\n",
        e.code, e.pc,
        e.xax, e.xbx, e.xcx, e.xdx,
        e.xsi, e.xdi, e.xbp, e.xsp);
        
    dr_write_file(log_file, buf, l);
    flush_buffer(data);

}

#ifdef _WIN32
static bool event_exception(void* drcontext, dr_exception_t* except)
{
    thread_data_t* data = drmgr_get_tls_field(drcontext, tls_index);
    dr_mcontext_t* mc = except->mcontext;

    bc_exception e = {
        except->record->ExceptionCode,
        (uintptr_t)mc->pc,
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

    bc_exception_trace e = {
        info->sig,
        (uintptr_t)mc->pc,
        mc->xax, mc->xbx, mc->xcx, mc->xdx,
        mc->xsi, mc->xdi, mc->xbp, mc->xsp
    };
    return DR_SIGNAL_DELIVER;
}
#endif

static void event_exit(void)
{
    if (out_file != INVALID_FILE)
    {
        dr_close_file(out_file);
    }
    dr_write_file(log_file, "App closed\n", 10);
    dr_flush_file(log_file);
    drmgr_unregister_tls_field(tls_index);
    drmgr_exit();
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char* argv[])
{

    bool custom_name = false;
    char fname[64];
    Header hdr;

    for (int i=0;i<argc;i++) {
        if (strcmp(argv[i], "-outfile") == 0 && i != argc - 1) {
            custom_name = true;
            sprintf(fname, "%s", argv[i+1]);
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

        app_name = dr_module_preferred_name(main_mod);
        const char* path = main_mod->full_path;

        char buf[64];
        int l = dr_snprintf(buf, sizeof(buf), "Starting analysis of target %s\n", app_name);
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
    if (out_file == INVALID_FILE) {
        dr_fprintf(STDERR, "Unable to open the output file!\n");
    }
    dr_write_file(out_file, &hdr, sizeof(Header));

    drmgr_register_thread_init_event(event_thread_init);
    drmgr_register_thread_exit_event(event_thread_exit);
    // drmgr_register_module_load_event(event_module_load);
    
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
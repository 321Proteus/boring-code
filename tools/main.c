#include <dr_api.h>
#include <drmgr.h>
#include <dr_events.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "dr_modules.h"
#include "dr_tools.h"
#include "header.h"

#ifdef X64
static const bool x64 = true;
#else
static const bool x64 = false;
#endif

#define CHUNK_SIZE 10000
static void* log_mutex;
static file_t out_file;
static app_pc buffer[CHUNK_SIZE];
static int buf_idx = 0;

static void flush_buffer()
{
    dr_mutex_lock(log_mutex);
    if (buf_idx > 0 && out_file != INVALID_FILE)
    {
        dr_write_file(out_file, buffer, buf_idx * sizeof(app_pc));
        buf_idx = 0;
    }
    dr_mutex_unlock(log_mutex);
}

static void record_bbl(app_pc pc)
{
    dr_mutex_lock(log_mutex);
    buffer[buf_idx++] = pc;
    dr_mutex_unlock(log_mutex);
    if (buf_idx >= CHUNK_SIZE) flush_buffer();
}

dr_emit_flags_t event_basic_block(void *drcontext, void *tag,
    instrlist_t *bb, instr_t *instr, char for_trace, char translating, void *user_data)
{
    instr_t* first = instrlist_first_app(bb);
    if (first == NULL)  return DR_EMIT_DEFAULT;

    app_pc pc = instr_get_app_pc(first);

    dr_insert_clean_call(drcontext, bb, first, (void*)record_bbl,
        false, 1, OPND_CREATE_INTPTR(pc));

    return DR_EMIT_DEFAULT;
}

// static bool event_exception(void* drcontext, dr_exception_t* except)
// {
//     flush_buffer();
//     return true;
// }

static void event_exit(void)
{
    flush_buffer();
    if (out_file != INVALID_FILE)
    {
        dr_close_file(out_file);
    }
    dr_mutex_destroy(log_mutex);
    drmgr_exit();
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char* argv[])
{

    bool custom_name = false;
    char fname[64];
    Header hdr;

    for (int i=0;i<argc-1;i++) {
        if (strcmp(argv[i], "-outfile") == 0) {
            custom_name = true;
            sprintf(fname, "%s", argv[i+1]);
            break;
        }
    }

    drmgr_init();
    dr_set_client_name("BoringTool Tracer", "http://dynamorio.org/");
    
    log_mutex = dr_mutex_create();

    module_data_t *main_mod = dr_get_main_module();

    if (main_mod) {
        const char* path = main_mod->full_path;
        dr_printf("Starting analysis of target %s\n", path);

        hdr = create_header(path, x64);
        hdr.base = (uint64_t)main_mod->start;

        if (!custom_name) sprintf(fname, "bcfunctions_%08x.bin", hdr.hash);
        dr_free_module_data(main_mod);
    } else {
        hdr = create_header(NULL, x64);
        hdr.base = 0;
        dr_printf("Warning: Couldn't find the main module, which probably indicates a broken or obfuscated binary. \
            Image base and CRC32 have been set to their default values.");
        if (!custom_name) sprintf(fname, "bcfunctions.bin");
    }

    out_file = dr_open_file(fname, DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
    if (out_file == INVALID_FILE) {
        dr_fprintf(STDERR, "Unable to open the output file!\n");
    }
    dr_write_file(out_file, &hdr, sizeof(Header));

    drmgr_register_bb_instrumentation_event(
        NULL,
        event_basic_block,
        NULL
    );
    dr_register_exit_event(event_exit);
    // drmgr_register_exception_event(event_exception);
}

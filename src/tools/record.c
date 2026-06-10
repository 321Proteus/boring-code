#include "record.h"
#include "dr_ir_opnd.h"
#include "main.h"
#include "core/format.h"
#include "dr_ir_macros.h"
#include "dr_ir_macros_x86.h"
#include "dr_ir_utils.h"
#include "dr_tools.h"
#include "drreg.h"
#include "drx.h"

void record_x64(void* drcontext, instrlist_t* bb, instr_t* first, app_pc pc,
    reg_id_t reg1, reg_id_t reg2
) {

    dr_insert_read_raw_tls(drcontext, bb, first,
        tls_raw_base, tls_raw_offset, reg1
    );

    instrlist_meta_preinsert(bb, first, INSTR_CREATE_mov_imm(drcontext,
        opnd_create_reg(reg2), OPND_CREATE_INTPTR(pc)
    ));

    instrlist_meta_preinsert(bb, first, INSTR_CREATE_shr(drcontext, 
        opnd_create_reg(reg2), OPND_CREATE_INT8(16)
    ));

    instrlist_meta_preinsert(bb, first, INSTR_CREATE_or(drcontext,
        opnd_create_reg(reg2), opnd_create_reg(reg1)
    ));

    drx_buf_insert_load_buf_ptr(drcontext, trace_buf, bb, first, reg1);

    drx_buf_insert_buf_store(drcontext, trace_buf, bb, first,
        reg1, DR_REG_NULL,
        opnd_create_reg(reg2), OPSZ_8, 0
    );

    drx_buf_insert_update_buf_ptr(drcontext, trace_buf, bb, first,
        reg1, DR_REG_NULL, sizeof(bc_block_trace_t)
    );

}

void record_x86(void* drcontext, instrlist_t* bb, instr_t* first, app_pc pc,
    reg_id_t reg1, reg_id_t reg2
) {

    dr_insert_read_raw_tls(drcontext, bb, first,
        tls_raw_base, tls_raw_offset, reg2
    );

    instrlist_meta_preinsert(bb, first, INSTR_CREATE_shl(drcontext,
        opnd_create_reg(reg_resize_to_opsz(reg2, OPSZ_4)), OPND_CREATE_INT8(16)
    ));

    drx_buf_insert_load_buf_ptr(drcontext, trace_buf, bb, first, reg1);

    drx_buf_insert_buf_store(drcontext, trace_buf, bb, first,
        reg1, DR_REG_NULL,
        OPND_CREATE_INT32((int32_t)(uintptr_t)pc), OPSZ_4, 0
    );

    drx_buf_insert_buf_store(drcontext, trace_buf, bb, first,
        reg1, DR_REG_NULL,
        opnd_create_reg(reg_resize_to_opsz(reg2, OPSZ_4)), OPSZ_4,
        offsetof(bc_block_trace_t, pc_mid)
    );

    drx_buf_insert_update_buf_ptr(drcontext, trace_buf, bb, first,
        reg1, DR_REG_NULL, sizeof(bc_block_trace_t)
    );

}

void emit_inline_record(void* drcontext, instrlist_t* bb, instr_t* first, app_pc pc) {

    reg_id_t reg1, reg2;

    if (drreg_reserve_register(drcontext, bb, first, NULL, &reg1) != DRREG_SUCCESS ||
        drreg_reserve_register(drcontext, bb, first, NULL, &reg2) != DRREG_SUCCESS ||
        drreg_reserve_aflags(drcontext, bb, first) != DRREG_SUCCESS) {
            dr_printf("Unable to reserve registers for block " PFX "\n", bb);
        // dr_insert_clean_call(drcontext, bb, first, (void*)record_bbl,
        //     false, 1, OPND_CREATE_INTPTR(pc));
        return;
    }

#ifdef X64
    record_x64(drcontext, bb, first, pc, reg1, reg2);
#else
    record_x86(drcontext, bb, first, pc, reg1, reg2);
#endif

    drreg_unreserve_register(drcontext, bb, first, reg1);
    drreg_unreserve_register(drcontext, bb, first, reg2);
    drreg_unreserve_aflags(drcontext, bb, first);

}
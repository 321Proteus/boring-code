#pragma once

#include "dr_defines.h"
#include "dr_ir_opnd.h"

void record_x64(void* drcontext, instrlist_t* bb, instr_t* first, app_pc pc, reg_id_t reg1, reg_id_t reg2);
void record_x86(void* drcontext, instrlist_t* bb, instr_t* first, app_pc pc, reg_id_t reg1, reg_id_t reg2);
void emit_inline_record(void* drcontext, instrlist_t* bb, instr_t* first, app_pc pc);
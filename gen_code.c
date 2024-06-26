#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "bof.h"
#include "instruction.h"
#include "id_use.h"
#include "code.h"
#include "gen_code.h"
#include "literal_table.h"
#include "utilities.h"
#include "regname.h"
#include "pl0.tab.h"

#define STACK_SPACE 4096

void gen_code_initialize() 
{
    literal_table_initialize();
}

static void gen_code_output_seq(BOFFILE bf, code_seq cs)
{
    while (!code_seq_is_empty(cs)) 
    {
        bin_instr_t inst = code_seq_first(cs)->instr;
        instruction_write_bin_instr(bf, inst);
        cs = code_seq_rest(cs);
    }
}

// Return a header appropriate for the give code
static BOFHeader gen_code_program_header(code_seq main_cs)
{
    BOFHeader ret;
    strncpy(ret.magic, "BOF", MAGIC_BUFFER_SIZE);
    ret.text_start_address = 0;
    ret.text_length = code_seq_size(main_cs) * BYTES_PER_WORD;
    int dsa = ret.text_length + BYTES_PER_WORD;
    ret.data_start_address = dsa;
    ret.data_length = literal_table_size() * BYTES_PER_WORD;
    int sba = dsa + ret.data_length + STACK_SPACE;
    ret.stack_bottom_addr = sba;
    return ret;
}

static void gen_code_output_literals(BOFFILE bf)
{
    literal_table_start_iteration();
    while (literal_table_iteration_has_next()) {
        word_type w = literal_table_iteration_next();
        // debug_print("Writing literal %f to BOF file\n", w);
        bof_write_word(bf, w);
    }
    literal_table_end_iteration(); // not necessary
}

void gen_code_output_program(BOFFILE bf, code_seq main_seq)
{
    BOFHeader bfh = gen_code_program_header(main_seq);
    bof_write_header(bf, bfh);
    gen_code_output_seq(bf, main_seq);
    gen_code_output_literals(bf);
    bof_close(bf);
}

void gen_code_program(BOFFILE bf, block_t prog)
{
    code_seq main_seq;
    main_seq = gen_code_var_decls(prog.var_decls);

    int var_num_bytes = (code_seq_size(main_seq) / 2) * BYTES_PER_WORD;

    main_seq = code_seq_concat(main_seq, gen_code_const_decls(prog.const_decls));
    main_seq = code_seq_concat(main_seq, code_save_registers_for_AR());
    main_seq = code_seq_concat(main_seq, gen_code_stmt(prog.stmt));
    main_seq = code_seq_concat(main_seq, code_restore_registers_from_AR());
    main_seq = code_seq_concat(main_seq, code_deallocate_stack_space(var_num_bytes));
    main_seq = code_seq_add_to_end(main_seq, code_exit());
    gen_code_output_program(bf, main_seq);
}

code_seq gen_code_block(block_t blk)
{
    code_seq ret = gen_code_const_decls(blk.const_decls);
    ret = code_seq_concat(ret, gen_code_var_decls(blk.var_decls));
    ret = code_seq_concat(ret, gen_code_stmt(blk.stmt));
    return ret;
}

code_seq gen_code_const_decls(const_decls_t cds)
{
    code_seq ret = code_seq_empty();
    const_decl_t *cdp = cds.const_decls;

    while (cdp != NULL)
    {
        ret = code_seq_concat(gen_code_const_decl(*cdp), ret);
        cdp = cdp->next;
    }
    return ret;
}

code_seq gen_code_const_decl(const_decl_t cd)
{
    return gen_code_const_defs(cd.const_defs);
}

code_seq gen_code_const_defs(const_defs_t cdfs)
{
    code_seq ret = code_seq_empty();
    const_def_t *cdfp = cdfs.const_defs;
    while (cdfp != NULL)
    {
        code_seq alloc_and_init = code_seq_singleton(code_addi(SP, SP, - BYTES_PER_WORD));
        alloc_and_init = code_seq_add_to_end(alloc_and_init, code_lw(GP, AT, literal_table_lookup(cdfp->number.text, cdfp->number.value)));
        alloc_and_init = code_seq_add_to_end(alloc_and_init, code_sw(SP, AT, 0));
        ret = code_seq_concat(alloc_and_init, ret);
        cdfp = cdfp->next;
    }
    return ret;
}

code_seq gen_code_var_decls(var_decls_t vds)
{
    code_seq ret = code_seq_empty();
    var_decl_t *vdp = vds.var_decls;

    while (vdp != NULL)
    {
        ret = code_seq_concat(gen_code_var_decl(*vdp), ret);
        vdp = vdp->next;
    }
    return ret;
}

code_seq gen_code_var_decl(var_decl_t vd)
{
    return gen_code_idents(vd.idents);
}

code_seq gen_code_idents(idents_t idents)
{
    code_seq ret = code_seq_empty();
    ident_t *idp = idents.idents;
    while (idp != NULL)
    {
        code_seq alloc_and_init = code_seq_singleton(code_addi(SP, SP, - BYTES_PER_WORD));
        alloc_and_init = code_seq_add_to_end(alloc_and_init, code_sw(SP, 0, 0));
        ret = code_seq_concat(alloc_and_init, ret);
        idp = idp->next;
    }
    return ret;
}

code_seq gen_code_stmt(stmt_t stmt)
{
    switch (stmt.stmt_kind)
    {
        case assign_stmt:
            return gen_code_assign_stmt(stmt.data.assign_stmt);
            break;
        case begin_stmt:
            return gen_code_begin_stmt(stmt.data.begin_stmt);
            break;
        case if_stmt:
            return gen_code_if_stmt(stmt.data.if_stmt);
            break;
        case while_stmt:
            return gen_code_while_stmt(stmt.data.while_stmt);
            break;
        case read_stmt:
            return gen_code_read_stmt(stmt.data.read_stmt);
            break;
        case write_stmt:
            return gen_code_write_stmt(stmt.data.write_stmt);
            break;
        case skip_stmt:
            return gen_code_skip_stmt(stmt.data.skip_stmt);
        default:
            bail_with_error("Call to gen_code_stmt with an AST that is not a statement!");
	        break;
    }
    return code_seq_empty();
}

code_seq gen_code_stmts(stmts_t stmts)
{
    code_seq ret = code_seq_empty();
    stmt_t *sp = stmts.stmts;
    while (sp != NULL) 
    {
        ret = code_seq_concat(ret, gen_code_stmt(*sp));
        sp = sp->next;
    }
    return ret;
}

code_seq gen_code_assign_stmt(assign_stmt_t stmt)
{
    code_seq ret;

    ret = gen_code_expr(*(stmt.expr));
    ret = code_seq_concat(ret, code_pop_stack_into_reg(V0));
    ret = code_seq_concat(ret, code_compute_fp(T9, stmt.idu->levelsOutward));
    unsigned int offset_count = id_use_get_attrs(stmt.idu)->offset_count;
    ret = code_seq_add_to_end(ret, code_sw(T9, V0, offset_count));

    return ret;
}

code_seq gen_code_begin_stmt(begin_stmt_t stmt)
{
    code_seq ret = code_seq_empty(); 
    // allocate space and initialize any variables in block
    ret = code_seq_concat(ret, gen_code_stmts(stmt.stmts));
    return ret;
}

code_seq gen_code_if_stmt(if_stmt_t stmt)
{
    code_seq ret = gen_code_condition(stmt.condition);

    code_seq then_stmt = gen_code_stmt(*(stmt.then_stmt));
    int then_len = code_seq_size(then_stmt);
    ret = code_seq_add_to_end(ret, code_beq(V0, 0, then_len));
    ret = code_seq_concat(ret, then_stmt);

    code_seq else_stmt = gen_code_stmt(*(stmt.else_stmt));
    int else_len = code_seq_size(else_stmt);
    ret = code_seq_add_to_end(ret, code_bne(V0, 0, else_len));
    return ret = code_seq_concat(ret, else_stmt);
}

code_seq gen_code_while_stmt(while_stmt_t stmt)
{
    code_seq ret = gen_code_condition(stmt.condition);
    stmt_t *sp = stmt.body;
    code_seq body = code_seq_empty();
    while (sp != NULL)
    {
        body = code_seq_concat(body, gen_code_stmt(*sp));
        sp = sp->next;
    }
    int body_len = code_seq_size(body);
    code_seq jump = code_jmp(stmt.condition.file_loc->line * BYTES_PER_WORD);
    int jump_len = code_seq_size(jump);
    ret = code_seq_add_to_end(ret, code_beq(V0, 0, body_len + jump_len));
    ret = code_seq_concat(ret, body);
    return code_seq_concat(ret, jump);
}

code_seq gen_code_read_stmt(read_stmt_t stmt)
{
    code_seq ret = code_seq_singleton(code_rch());
    ret = code_seq_concat(ret, code_compute_fp(T9, stmt.idu->levelsOutward));
    unsigned int offset_count = id_use_get_attrs(stmt.idu)->offset_count;
    ret = code_seq_add_to_end(ret, code_seq_singleton(code_sw(T9, V0, offset_count)));
    return ret;
}

code_seq gen_code_write_stmt(write_stmt_t stmt)
{
    code_seq ret = gen_code_expr(stmt.expr);
    ret = code_seq_concat(ret, code_pop_stack_into_reg(A0));
    ret = code_seq_add_to_end(ret, code_pint());
    return ret; 
} 

code_seq gen_code_skip_stmt(skip_stmt_t stmt)
{
    return code_seq_empty();
}

code_seq gen_code_condition(condition_t cond)
{
    switch (cond.cond_kind)
    {
        case ck_odd:
            return gen_code_odd_condition(cond.data.odd_cond);
            break;
        case ck_rel:
            return gen_code_rel_op_condition(cond.data.rel_op_cond);
            break;
        default:
            bail_with_error("Unexpected type_tag (%d) in scope_check_cond!",
			cond.cond_kind);
	        break;
    }
    return code_seq_empty();
}

code_seq gen_code_odd_condition(odd_condition_t cond)
{
    code_seq ret = gen_code_expr(cond.expr);
    ret = code_seq_concat(ret, code_pop_stack_into_reg(V0));
    ret = code_seq_add_to_end(ret, code_andi(V0, V0, 1));
    ret = code_seq_add_to_end(ret, code_add(0, 0, V0)); // Put false in V0
    ret = code_seq_add_to_end(ret, code_beq(0, 0, 1)); // Skip next instr
    ret = code_seq_add_to_end(ret, code_addi(0, V0, 1)); // Put true in V0
    ret = code_seq_concat(ret, code_push_reg_on_stack(V0));
    return ret;
}

code_seq gen_code_rel_op_condition(rel_op_condition_t cond)
{
    code_seq ret = gen_code_expr(cond.expr1);
    ret = code_seq_concat(ret, gen_code_expr(cond.expr2));
    ret = code_seq_concat(ret, code_pop_stack_into_reg(AT));
    ret = code_seq_concat(ret, code_pop_stack_into_reg(V0));
    ret = code_seq_concat(ret, gen_code_rel_op(cond.rel_op));
    return ret;
}

code_seq gen_code_rel_op(token_t rel_op)
{
    code_seq do_op = code_seq_empty();
    switch(rel_op.code)
    {
        case eqsym:
            do_op = code_seq_singleton(code_beq(V0, AT, 2));
            break;
        case neqsym:
            do_op = code_seq_singleton(code_bne(V0, AT, 2));
            break;
        case ltsym:
            do_op = code_seq_singleton(code_sub(V0, AT, V0));
	        do_op = code_seq_add_to_end(do_op, code_bltz(V0, 2));
            break;
        case leqsym:
            do_op = code_seq_singleton(code_sub(V0, AT, V0));
            do_op = code_seq_add_to_end(do_op, code_blez(V0, 2));
            break;
        case gtsym:
            do_op = code_seq_singleton(code_sub(V0, AT, V0));
	        do_op = code_seq_add_to_end(do_op, code_bgtz(V0, 2));
            break;
        case geqsym:
            do_op = code_seq_singleton(code_sub(V0, AT, V0));
	        do_op = code_seq_add_to_end(do_op, code_bgez(V0, 2));
            break;
        default:
            bail_with_error("Unknown token code (%d) in gen_code_rel_op",
			rel_op.code);
	        break;
    }
    code_seq ret = do_op;
    ret = code_seq_add_to_end(ret, code_add(0, 0, V0)); // Put false in V0
    ret = code_seq_add_to_end(ret, code_beq(0, 0, 1)); // Skip next instr
    ret = code_seq_add_to_end(ret, code_addi(0, V0, 1)); // Put true in V0
    ret = code_seq_concat(ret, code_push_reg_on_stack(V0));
    return ret;
}

code_seq gen_code_expr(expr_t exp)
{
    switch (exp.expr_kind)
    {
        case expr_bin:
            return gen_code_binary_op_expr(exp.data.binary);
            break;
        case expr_ident:
            return gen_code_ident(exp.data.ident);
            break;
        case expr_number:
            return gen_code_number(exp.data.number);
            break;
        default:
            bail_with_error("Unexpected expr_kind_e (%d) in gen_code_expr",
			exp.expr_kind);
	        break;
    }
    return code_seq_empty();
}

code_seq gen_code_op(token_t op)
{
    switch (op.code)
    {
        case eqsym: case neqsym:
        case ltsym: case leqsym:
        case gtsym: case geqsym:
            return gen_code_rel_op(op);
            break;
        case plussym: case minussym:
        case multsym: case divsym:
            return gen_code_arith_op(op);
            break;
        default:
            bail_with_error("Unknown token code (%d) in gen_code_op",
			op.code);
	        break;
    }
    return code_seq_empty();
}

code_seq gen_code_binary_op_expr(binary_op_expr_t exp)
{
    code_seq ret = gen_code_expr(*(exp.expr1));
    ret = code_seq_concat(ret, gen_code_expr(*(exp.expr2)));
    ret = code_seq_concat(ret, gen_code_op(exp.arith_op));
    return ret;
}

code_seq gen_code_arith_op(token_t arith_op)
{
    code_seq ret = code_pop_stack_into_reg(T2);
    ret = code_seq_concat(ret, code_pop_stack_into_reg(T1));
    
    code_seq do_op = code_seq_empty();
    switch (arith_op.code)
    {
        case plussym:
            do_op = code_seq_add_to_end(do_op, code_add(T1, T2, T1));
            break;
        case minussym:
            do_op = code_seq_add_to_end(do_op, code_sub(T1, T2, T1));
            break;
        case multsym:
            do_op = code_seq_add_to_end(do_op, code_mul(T1, T2));
            do_op = code_seq_add_to_end(do_op, code_mflo(T1));
            break;
        case divsym:
            do_op = code_seq_add_to_end(do_op, code_div(T1, T2));
            do_op = code_seq_add_to_end(do_op, code_mflo(T1));
            break;
        default:
            bail_with_error("Unexpected arithOp (%d) in gen_code_arith_op",
			arith_op.code);
	        break;
    }
    do_op = code_seq_concat(do_op, code_push_reg_on_stack(T1));
    return code_seq_concat(ret, do_op);
}

code_seq gen_code_ident(ident_t id)
{
    code_seq ret = code_compute_fp(T9, id.idu->levelsOutward);
    unsigned int offset_count = id_use_get_attrs(id.idu)->offset_count;
    ret = code_seq_add_to_end(ret, code_lw(T9, V0, offset_count));
    return code_seq_concat(ret, code_push_reg_on_stack(V0));
}

code_seq gen_code_number(number_t num)
{
    unsigned int global_offset = literal_table_lookup(num.text, num.value);
    return code_seq_concat(code_seq_singleton(code_lw(GP, V0, global_offset)), code_push_reg_on_stack(V0));

}

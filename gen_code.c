#include <stdio.h>
#include <assert.h>
#include "ast.h"
#include "bof.h"
#include "instruction.h"
#include "id_use.h"
#include "code.h"
#include "gen_code.h"
#include "literal_table.h"

void gen_code_initialize() 
{
    literal_table_initialize();
}

void gen_code_output_program(BOFFILE bf, code_seq main_seq)
{
    // BOFHeader bfh = gen_code_program_header(main_cs);
    // bof_write_header(bf, bfh);
    gen_code_output_seq(bf, main_seq);
    // gen_code_output_literals(bf);
    bof_close(bf);
}

void gen_code_program(BOFFILE bf, block_t prog)
{
    code_seq main_seq;
    // main_seq = gen_code_var_decls(prog.var_decls);

    int var_num_bytes = (code_seq_size(main_seq) / 2) * BYTES_PER_WORD;

    // main_seq = code_seq_concat(main_seq, code_save_registers_for_AR());
    main_cs = code_seq_concat(main_cs, gen_code_stmt(prog.stmt));
    // main_seq = code_seq_concat(main_seq, code_restore_registers_from_AR());
    // main_seq = code_seq_concat(main_seq, code_deallocate_stack_space(var_num_bytes));
    main_seq = code_seq_add_to_end(main_seq, code_exit());
    gen_code_output_program(bf, main_seq);
}

code_seq gen_code_block(block_t blk)
{
    
}

code_seq gen_code_const_decls(const_decls_t cds)
{

}

code_seq gen_code_const_decl(const_decl_t cd)
{

}

code_seq gen_code_const_defs(const_defs_t cdfs)
{

}

code_seq gen_code_const_def(const_def_t cdf)
{

}

code_seq gen_code_var_decls(var_decls_t vds)
{

}

code_seq gen_code_var_decl(var_decl_t vd)
{

}

code_seq gen_code_idents(idents_t idents)
{

}

code_seq gen_code_proc_decls(proc_decls_t pds)
{

}

code_seq gen_code_proc_decl(proc_decl_t pd)
{

}

code_seq gen_code_stmts(stmts_t stmts)
{

}

code_seq gen_code_stmt(stmt_t stmt)
{
    switch (stmt.stmt_kind)
    {
        case assign_stmt:
            return gen_code_assign_stmt(stmt.data.assign_stmt);
            break;
        case call_stmt:
            return gen_code_call_stmt(stmt.data.call_stmt);
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
}

code_seq gen_code_assign_stmt(assign_stmt_t stmt)
{
    code_seq ret;

    ret = gen_code_expr(*(stmt.expr));

}

code_seq gen_code_call_stmt(call_stmt_t stmt)
{

}

code_seq gen_code_begin_stmt(begin_stmt_t stmt)
{
    code_seq block_seq;
    // allocate space and initialize any variables in block
    // block_seq = gen_code_var_decls(stmt.var_decls);
    int vars_len_in_bytes = (code_seq_size(block_seq) / 2) * BYTES_PER_WORD;
    /*
    ret = code_seq_add_to_end(ret, code_add(0, FP, A0));
    ret = code_seq_concat(ret, code_save_registers_for_AR());
    ret = code_seq_concat(ret, gen_code_stmts(stmt.stmts));
    ret = code_seq_concat(ret, code_restore_registers_from_AR());
    ret = code_seq_concat(ret, code_deallocate_stack_space(vars_len_in_bytes));
    */

    gen_code_stmts(stmt.stmts);
    return block_seq;
}

code_seq gen_code_if_stmt(if_stmt_t stmt)
{

}

code_seq gen_code_while_stmt(while_stmt_t stmt)
{

}

code_seq gen_code_read_stmt(read_stmt_t stmt)
{

}

code_seq gen_code_write_stmt(write_stmt_t stmt)
{
    
}

code_seq gen_code_skip_stmt(skip_stmt_t stmt)
{

}

code_seq gen_code_condition(condition_t cond)
{

}

code_seq gen_code_odd_condition(odd_condition_t cond)
{

}

code_seq gen_code_rel_op_condition(rel_op_condition_t cond)
{

}

code_seq gen_code_rel_op(token_t rel_op)
{

}

code_seq gen_code_expr(expr_t exp)
{

}

code_seq gen_code_binary_op_expr(binary_op_expr_t exp)
{

}

code_seq gen_code_arith_op(token_t arith_op)
{

}

code_seq gen_code_ident(ident_t id)
{

}

code_seq gen_code_number(number_t num)
{

}
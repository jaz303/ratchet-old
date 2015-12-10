#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct ast_node ast_node_t;

#include "util.inc.c"
#include "val.inc.c"
#include "ast.inc.c"
#include "lexer.inc.c"
#include "symboltable.inc.c"
#include "parser.inc.c"

/**
 * Next steps
 *
 * 1. decorate AST
 * 2. replace naive register allocation with Sethi-Ullman
 */

enum {
    OP_PRINT    = (1 << 26),
    OP_HALT     = (2 << 26),
    OP_ADD      = (3 << 26),
    OP_SUB      = (4 << 26),
    OP_LOADK    = (5 << 26),
    OP_COPY     = (6 << 26),
    OP_CALL     = (7 << 26),
    OP_LT       = (8 << 26),
    OP_JMP      = (9 << 26),
    OP_JMPF     = (10 << 26)
};

typedef uint32_t inst_t;

typedef struct {
    val_t *constants;
    inst_t *code;
    int pi;
    int ki;
    int reg;
} code_t;

val_t mul(val_t *args, int nargs) {
    printf("mul: %d %d %d\n", args[0].ival, args[1].ival, args[2].ival);
    return mk_int(args[0].ival * args[1].ival * args[2].ival);
}

int compile_exp(val_t exp, code_t *code);
void compile_print(val_t exp, code_t *co);
void compile_while(val_t node, code_t *co);

// Register allocation:
// https://en.wikipedia.org/wiki/Sethi%E2%80%93Ullman_algorithm
// https://lambda.uta.edu/cse5317/fall02/notes/node40.html
// http://www.christianwimmer.at/Publications/Wimmer10a/Wimmer10a.pdf

void compile_statements(val_t stmt, code_t *code) {
    while (!nil_p(stmt)) {
        val_t subj = ((ast_list_t*)stmt.ast)->exp;
        switch (ast_type(subj)) {
            case AST_PRINT:
                compile_print(subj, code);
                break;
            case AST_WHILE:
                compile_while(subj, code);
                break;
            default:
                compile_exp(subj, code);
                break;
        }
        stmt = ((ast_list_t*)stmt.ast)->next;
    }
}

int compile_exp(val_t val, code_t *co) {
    // printf("compile exp: %d\n", ast_type(val));
    if (val.type == T_IDENT) {
        return val.ival;
    } else if (val.type == T_INT) {
        int constant = co->ki++;
        co->constants[constant] = val;
        int dst = co->reg++;
        co->code[co->pi++] = OP_LOADK | (dst << 16) | constant;
        return dst;
    } else if (val.type == T_AST) {
        if (val.ast->type & AST_BINOP_MASK) {
            ast_binop_t *op = (ast_binop_t*)val.ast;
            if (op->base.type == AST_ADD
                || op->base.type == AST_SUB
                || op->base.type == AST_LT) {
                int lreg = compile_exp(op->l, co);
                int rreg = compile_exp(op->r, co);
                int oreg = co->reg++;
                int opcodes[] = {
                    0,
                    OP_ADD,
                    OP_SUB,
                    OP_LT
                };
                int opcode = opcodes[op->base.type & ~AST_BINOP_MASK];
                co->code[co->pi++] = opcode | (oreg << 16) | (lreg << 8) | rreg;
                return oreg;
            } else if (op->base.type == AST_ASSIGN) {
                int dst = op->l.ival;
                int src = compile_exp(op->r, co);
                co->code[co->pi++] = OP_COPY | (dst << 16) | src;
                return dst;
            }
        } else if (val.ast->type == AST_CALL) {
            ast_call_t *call = (ast_call_t*)val.ast;
            int nargs = ast_list_len(call->args);
            int r_callee = co->reg++;
            int r_argbase = co->reg;
            co->reg += nargs;
            int r_callee_val = compile_exp(call->callee, co);
            co->code[co->pi++] = OP_COPY | (r_callee << 16) | r_callee_val;
            val_t thisarg = call->args;
            int argix = 0;
            while (!nil_p(thisarg)) {
                int r_arg = compile_exp(((ast_list_t*)thisarg.ast)->exp, co);
                co->code[co->pi++] = OP_COPY | ((r_argbase + argix) << 16) | r_arg;
                argix++;
                thisarg = ((ast_list_t*)thisarg.ast)->next;
            }
            int r_res = co->reg++;
            co->code[co->pi++] = OP_CALL | (r_callee << 16) | (nargs << 8) | r_res;
            return r_res;
        }
        printf("unknown AST type for expression: %d\n", val.ast->type);
    }
    return -1;
}

void compile_print(val_t exp, code_t *co) {
    int reg = compile_exp(((ast_print_t*)exp.ast)->exp, co);
    co->code[co->pi++] = OP_PRINT | reg;
}

void compile_while(val_t node, code_t *co) {
    int start = co->pi;
    int reg = compile_exp(((ast_while_t*)node.ast)->cond, co);
    int jumper = co->pi++;
    compile_statements(((ast_while_t*)node.ast)->body, co);
    co->code[co->pi++] = OP_JMP | start;
    co->code[jumper] = OP_JMPF | (reg << 16) | co->pi;
}

code_t* compile(val_t program, int nlocals) {
    code_t *co = malloc(sizeof(code_t));
    co->constants = malloc(sizeof(val_t) * 128);
    co->code = malloc(sizeof(inst_t) * 128);
    co->pi = 0;
    co->ki = 0;
    co->reg = nlocals;

    compile_statements(program, co);

    co->code[co->pi++] = OP_HALT;

    return co;
}

void run(code_t *co) {
    int ip = 0;
    val_t reg[128];

    reg[2].type = T_FOREIGN_FN;
    reg[2].fn = mul;

    while (1) {
        inst_t op = co->code[ip++];
        printf("op: 0x%x\n", op);
        switch (op & 0xfc000000) {
            case OP_PRINT:
                {
                    int r = op & 0xFF;
                    printf("print: %d\n", reg[r].ival);
                }
                break;
            case OP_ADD:
                {
                    int rd = (op >> 16) & 0xFF;
                    int r2 = (op >>  8) & 0xFF;
                    int r3 = (op >>  0) & 0xFF;
                    reg[rd].ival = reg[r2].ival + reg[r3].ival;
                }
                break;
            case OP_SUB:
                {
                    int rd = (op >> 16) & 0xFF;
                    int r2 = (op >>  8) & 0xFF;
                    int r3 = (op >>  0) & 0xFF;
                    reg[rd].ival = reg[r2].ival - reg[r3].ival;
                }
                break;
            case OP_LOADK:
                {
                    int r = (op >> 16) & 0xFF;
                    int k = (op >>  0) & 0xFFFF;
                    reg[r] = co->constants[k];
                }
                break;
            case OP_COPY:
                {
                    int rd = (op >> 16) & 0xFF;
                    int rs = (op >>  0) & 0xFF;
                    reg[rd] = reg[rs];
                }
                break;
            case OP_CALL:
                {
                    int base = (op >> 16) & 0xFF;
                    int nargs = (op >> 8) & 0xFF;
                    int result = op & 0xFF;
                    reg[result] = reg[base].fn(&reg[base+1], nargs);
                }
                break;
            case OP_LT:
                {
                    int rd = (op >> 16) & 0xFF;
                    int r2 = (op >>  8) & 0xFF;
                    int r3 = (op >>  0) & 0xFF;
                    reg[rd].type = (reg[r2].ival < reg[r3].ival)
                        ? T_TRUE
                        : T_FALSE;
                }
                break;
            case OP_JMP:
                {
                    ip = op & 0x00FFFFFF;
                }
                break;
            case OP_JMPF:
                {
                    if (!truthy_p(reg[(op >> 16) & 0xFF])) {
                        ip = op & 0x0000FFFF;
                    }
                }
                break;
            case OP_HALT:
                {
                    printf("execution terminated\n");    
                }
                return;
        } 
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <sourcefile>\n", argv[0]);
        return 1;
    }

    char *source = readfile(argv[1]);
    if (!source) {
        fprintf(stderr, "unable to read source file: %s\n", argv[1]);
        return 1;
    }

    rt_parser_t parser;
    rt_lexer_init(&parser.lexer, source);
    rt_parser_init(&parser);

    val_t mod = rt_parse_module(&parser);

    free(source);

    if (nil_p(mod)) {
        fprintf(stderr, "parse error: %s\n", parser.error);
        return 1;
    }

    // TODO: compile, run
}

// int main(int argc, char *argv[]) {

//     // a = 10
//     // b = a + 5
//     // print b

//     // val_t program = mk_ast_list(
//     //     mk_ast_binop(AST_ASSIGN, mk_ident(0), mk_int(10)),
//     //     mk_ast_list(
//     //         mk_ast_binop(AST_ASSIGN,
//     //             mk_ident(1),
//     //             mk_ast_binop(AST_ADD,
//     //                 mk_ident(0),
//     //                 mk_int(5)
//     //             )
//     //         ),
//     //         mk_ast_list(
//     //             mk_ast_print(
//     //                 mk_ast_call(
//     //                     mk_ident(2),
//     //                     mk_ast_list(
//     //                         mk_int(2),
//     //                         mk_ast_list(
//     //                             mk_ident(0),
//     //                             mk_ast_list(
//     //                                 mk_ident(1),
//     //                                 mk_nil()
//     //                             )
//     //                         )
//     //                     )
//     //                 )
//     //             ),
//     //             mk_nil()
//     //         )
//     //     )
//     // );

//     val_t program = mk_ast_list(
//         mk_ast_binop(AST_ASSIGN, mk_ident(0), mk_int(0)),
//         mk_ast_list(
//             mk_ast_while(
//                 mk_ast_binop(AST_LT, mk_ident(0), mk_int(10)),
//                 mk_ast_list(
//                     mk_ast_print(mk_ident(0)),
//                     mk_ast_list(
//                         mk_ast_binop(AST_ASSIGN,
//                             mk_ident(0),
//                             mk_ast_binop(AST_ADD,
//                                 mk_ident(0),
//                                 mk_int(1)
//                             )
//                         ),
//                         mk_nil()
//                     )
//                 )
//             ),
//             mk_nil()
//         )
//     );

//     code_t *code = compile(program, 3);

//     run(code);

//     return 0;

// }

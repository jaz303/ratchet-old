#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct ast_node ast_node_t;

#include "util.inc.cpp"
#include "types.inc.cpp"
#include "val.inc.cpp"
#include "ast.inc.cpp"
#include "lexer.inc.cpp"
#include "intern.inc.cpp"
#include "parser.inc.cpp"

/**
 * Next steps
 *
 * 1. decorate AST
 * 2. replace naive register allocation with Sethi-Ullman
 */

typedef uint32_t inst_t;

typedef struct {
    val_t *constants;
    inst_t *code;
    int pi;
    int ki;
    int reg;
} code_t;

val_t p1(val_t *args, int nargs) {
    printf("Hello from P1: %d\n", args[0].ival);
    return mk_nil();
}

val_t p2(val_t *args, int nargs) {
    printf("Hello from P2: %d\n", args[0].ival);
    return mk_nil();
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
        if (val.ast->type == AST_BIN_OP) {
            ast_binop_t *op = (ast_binop_t*)val.ast;
            if (op->op & OPERATOR_SIMPLE_BINOP_MASK) {
                int lreg = compile_exp(op->l, co);
                int rreg = compile_exp(op->r, co);
                int oreg = co->reg++;
                opcode_t opcode = rt_simple_binop_opcodes[op->op & ~OPERATOR_SIMPLE_BINOP_MASK];
                co->code[co->pi++] = opcode | (oreg << 16) | (lreg << 8) | rreg;
                return oreg;
            } else if (op->op == OPERATOR_ASSIGN) {
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
    code_t *co = (code_t*)malloc(sizeof(code_t));
    co->constants = (val_t*)malloc(sizeof(val_t) * 128);
    co->code = (inst_t*)malloc(sizeof(inst_t) * 128);
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

    reg[1].type = T_FOREIGN_FN;
    reg[1].fn = p1;
    reg[2].type = T_FOREIGN_FN;
    reg[2].fn = p2;

    while (1) {
        inst_t op = co->code[ip++];
        // printf("op: 0x%x\n", op);
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
    rt_intern_init();

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

    printf("node type: %d\n", mod.ast->type);

    free(source);

    if (nil_p(mod)) {
        fprintf(stderr, "parse error: %s\n", parser.error);
        return 1;
    }

    code_t *code = compile(mod, 3);
    run(code);
}

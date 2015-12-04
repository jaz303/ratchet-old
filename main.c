#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Next steps
 *
 * 1. decorate AST
 * 2. replace naive register allocation with Sethi-Ullman
 */

enum {
    T_NIL,
    T_AST,
    T_INT,
    T_IDENT,
    T_FOREIGN_FN
};

enum {
    AST_STMTS       = 0x01,
    AST_PRINT       = 0x02,
    AST_IDENT       = 0x03,
    AST_CALL        = 0x04,

    AST_ADD         = 0x81,
    AST_SUB         = 0x82,
    AST_ASSIGN      = 0x83
};

const int AST_BINOP_MASK = 0x80;

typedef struct ast_node ast_node_t;

typedef struct val val_t;

typedef val_t (*foreign_fn_f)(val_t *args, int nargs);

struct val {
    int type;
    union {
        int ival;
        ast_node_t *ast;
        foreign_fn_f fn;
    };
};

struct ast_node {
    int type;
};

typedef struct ast_call {
    ast_node_t base;
    val_t callee;
    val_t args;
} ast_call_t;

typedef struct ast_print {
    ast_node_t base;
    val_t exp;
} ast_print_t;

typedef struct ast_stmts ast_stmts_t;

struct ast_stmts {
    ast_node_t base;
    val_t stmt;
    val_t next;
};

typedef struct ast_binop {
    ast_node_t base;
    val_t l;
    val_t r;
} ast_binop_t;

enum {
    OP_PRINT    = (1 << 26),
    OP_HALT     = (2 << 26),
    OP_ADD      = (3 << 26),
    OP_SUB      = (4 << 26),
    OP_LOADK    = (5 << 26),
    OP_COPY     = (6 << 26),
    OP_CALL     = (7 << 26)
};

val_t mk_nil() {
    val_t out = { .type = T_NIL };
    return out;
}

val_t mk_int(int val) {
    val_t out;
    out.type = T_INT;
    out.ival = val;
    return out;
}

val_t mk_ident(int id) {
    val_t out;
    out.type = T_IDENT;
    out.ival = id;
    return out;
}

#define ALLOC_AST(struct_type, tag) \
    struct_type *node = malloc(sizeof(struct_type)); \
    ((ast_node_t*)node)->type = tag; \
    val_t val = { .type = T_AST, .ast = (ast_node_t*) node }

val_t mk_ast_stmt(val_t stmt, val_t next) {
    ALLOC_AST(ast_stmts_t, AST_STMTS);
    node->stmt = stmt;
    node->next = next;
    return val;
}

val_t mk_ast_call(val_t callee, val_t args) {
    ALLOC_AST(ast_call_t, AST_CALL);
    node->callee = callee;
    node->args = args;
    return val;
}

val_t mk_ast_print(val_t exp) {
    ALLOC_AST(ast_print_t, AST_PRINT);
    node->exp = exp;
    return val;
}

val_t mk_ast_binop(int type, val_t l, val_t r) {
    ALLOC_AST(ast_binop_t, type);
    node->l = l;
    node->r = r;
    return val;
}

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

int nil_p(val_t v) {
    return v.type == T_NIL;
}

int ast_type(val_t v) {
    return v.ast->type;
}

int ast_list_len(val_t v) {
    int len = 0;
    while (!nil_p(v)) {
        len++;
        v = ((ast_stmts_t*)v.ast)->next;
    }
    return len;
}

int compile_exp(val_t exp, code_t *code);
void compile_print(val_t exp, code_t *co);

// Register allocation:
// https://en.wikipedia.org/wiki/Sethi%E2%80%93Ullman_algorithm
// https://lambda.uta.edu/cse5317/fall02/notes/node40.html
// http://www.christianwimmer.at/Publications/Wimmer10a/Wimmer10a.pdf

void compile_statements(val_t stmt, code_t *code) {
    while (!nil_p(stmt)) {
        val_t subj = ((ast_stmts_t*)stmt.ast)->stmt;
        switch (ast_type(subj)) {
            case AST_PRINT:
                compile_print(subj, code);
                break;
            default:
                compile_exp(subj, code);
                break;
        }
        stmt = ((ast_stmts_t*)stmt.ast)->next;
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
                || op->base.type == AST_SUB) {
                int lreg = compile_exp(op->l, co);
                int rreg = compile_exp(op->r, co);
                int oreg = co->reg++;
                int opcodes[] = {
                    0,
                    OP_ADD,
                    OP_SUB
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
                int r_arg = compile_exp(((ast_stmts_t*)thisarg.ast)->stmt, co);
                co->code[co->pi++] = OP_COPY | ((r_argbase + argix) << 16) | r_arg;
                argix++;
                thisarg = ((ast_stmts_t*)thisarg.ast)->next;
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
            case OP_HALT:
                {
                    printf("execution terminated\n");    
                }
                return;
        } 
    }
}

int main(int argc, char *argv[]) {

    // a = 10
    // b = a + 5
    // print b

    val_t program = mk_ast_stmt(
        mk_ast_binop(AST_ASSIGN, mk_ident(0), mk_int(10)),
        mk_ast_stmt(
            mk_ast_binop(AST_ASSIGN,
                mk_ident(1),
                mk_ast_binop(AST_ADD,
                    mk_ident(0),
                    mk_int(5)
                )
            ),
            mk_ast_stmt(
                mk_ast_print(
                    mk_ast_call(
                        mk_ident(2),
                        mk_ast_stmt(
                            mk_int(2),
                            mk_ast_stmt(
                                mk_ident(0),
                                mk_ast_stmt(
                                    mk_ident(1),
                                    mk_nil()
                                )
                            )
                        )
                    )
                ),
                mk_nil()
            )
        )
    );

    code_t *code = compile(program, 3);

    run(code);

    return 0;

}

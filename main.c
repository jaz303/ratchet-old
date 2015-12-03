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
    OP_PRINT    = (1 << 26),
    OP_HALT     = (2 << 26),
    OP_ADD      = (3 << 26),
    OP_LOADK    = (4 << 26),
    OP_COPY     = (5 << 26)
};

enum {
    AST_STMTS,
    AST_PRINT,
    AST_ASSIGN,
    AST_LITERAL,
    AST_ADD,
    AST_IDENT
};

enum {
    T_NIL,
    T_AST,
    T_INT,
    T_IDENT
};

typedef struct val val_t;

struct val {
    int type;
    union {
        int ival;
    };
};

typedef struct {
    int type;
    val_t o1;
    val_t o2;
    val_t o3;
} ast_node_t;

ast_node_t ast_pool[32768];
int next_free_ast_node = 1;

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

val_t mk_ast1(int type, val_t n1) {
    val_t out;
    out.type = T_AST;
    int node_ix = out.ival = next_free_ast_node++;
    ast_pool[node_ix].type = type;
    ast_pool[node_ix].o1 = n1;
    return out;
}

val_t mk_ast2(int type, val_t n1, val_t n2) {
    val_t out;
    out.type = T_AST;
    int node_ix = out.ival = next_free_ast_node++;
    ast_pool[node_ix].type = type;
    ast_pool[node_ix].o1 = n1;
    ast_pool[node_ix].o2 = n2;
    return out;
}

val_t mk_ast3(int type, val_t n1, val_t n2, val_t n3) {
    val_t out;
    out.type = T_AST;
    int node_ix = out.ival = next_free_ast_node++;
    ast_pool[node_ix].type = type;
    ast_pool[node_ix].o1 = n1;
    ast_pool[node_ix].o2 = n2;
    ast_pool[node_ix].o3 = n3;
    return out;
}

#define MK_AST1(type, n1) mk_ast1(type, n1)
#define MK_AST2(type, n1, n2) mk_ast2(type, n1, n2)
#define MK_AST3(type, n1, n2, n3) mk_ast3(type, n1, n2, n3)

typedef uint32_t inst_t;

typedef struct {
    val_t *constants;
    inst_t *code;
    int pi;
    int ki;
    int reg;
} code_t;

int compile_exp(int ast_node, code_t *code);
int compile_assign(int ast_node, code_t *code);
void compile_print(int ast_node, code_t *code);
int compile_literal(int ast_node, code_t *code);
int compile_add(int ast_node, code_t *code);

#define AST_NEXT(n) ast_node = ast_pool[ast_node].o##n.ival

// Register allocation:
// https://en.wikipedia.org/wiki/Sethi%E2%80%93Ullman_algorithm
// https://lambda.uta.edu/cse5317/fall02/notes/node40.html
// http://www.christianwimmer.at/Publications/Wimmer10a/Wimmer10a.pdf

void compile_statements(int ast_node, code_t *code) {
    while (ast_node) {
        switch (ast_pool[ast_node].type) {
            case AST_PRINT:
                compile_print(ast_node, code);
                AST_NEXT(2);
                break;
            case AST_ASSIGN:
                compile_assign(ast_node, code);
                AST_NEXT(3);
                break;
            default:
                printf("unknown node type: %d\n", ast_pool[ast_node].type);
                break;
        }
    }
}

int compile_exp(int ast_node, code_t *co) {
    printf("compile exp: %d\n", ast_pool[ast_node].type);
    switch (ast_pool[ast_node].type) {
        case AST_LITERAL:
            return compile_literal(ast_node, co);
        case AST_ADD:
            return compile_add(ast_node, co);
        case AST_IDENT:
            return ast_pool[ast_node].o1.ival;
        default:
            printf("unknown expression type: %d\n", ast_pool[ast_node].type);
            return 0;
    }
}

int compile_assign(int ast_node, code_t *co) {
    int dst = ast_pool[ast_pool[ast_node].o1.ival].o1.ival;
    int src = compile_exp(ast_pool[ast_node].o2.ival, co);
    co->code[co->pi++] = OP_COPY | (dst << 16) | src;
    return dst;
}

void compile_print(int ast_node, code_t *co) {
    int reg = compile_exp(ast_pool[ast_node].o1.ival, co);
    co->code[co->pi++] = OP_PRINT | reg;
}

int compile_literal(int ast_node, code_t *co) {
    int constant = co->ki++;
    co->constants[constant] = ast_pool[ast_node].o1;
    int dst = co->reg++;
    co->code[co->pi++] = OP_LOADK | (dst << 16) | constant;
    return dst;
}

int compile_add(int ast_node, code_t *co) {
    int lreg = compile_exp(ast_pool[ast_node].o1.ival, co);
    int rreg = compile_exp(ast_pool[ast_node].o2.ival, co);
    int oreg = co->reg++;
    co->code[co->pi++] = OP_ADD | (oreg << 16) | (lreg << 8) | rreg;
    return oreg;
}

code_t* compile(val_t program, int nlocals) {
    code_t *co = malloc(sizeof(code_t));
    co->constants = malloc(sizeof(val_t) * 128);
    co->code = malloc(sizeof(inst_t) * 128);
    co->pi = 0;
    co->ki = 0;
    co->reg = nlocals;

    compile_statements(ast_pool[program.ival].o1.ival, co);

    co->code[co->pi++] = OP_HALT;

    return co;
}

void run(code_t *co) {
    int ip = 0;
    val_t reg[128];
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

    val_t program = MK_AST1(AST_STMTS,
        MK_AST3(AST_ASSIGN,
            MK_AST1(AST_IDENT, mk_ident(0)),
            MK_AST1(AST_LITERAL, mk_int(10)),
            MK_AST3(AST_ASSIGN,
                MK_AST1(AST_IDENT, mk_ident(1)),
                MK_AST2(AST_ADD,
                    MK_AST1(AST_IDENT, mk_ident(0)),
                    MK_AST1(AST_LITERAL, mk_int(5))
                ),
                MK_AST2(AST_PRINT,
                    MK_AST1(AST_IDENT, mk_ident(1)),
                    mk_nil()
                )
            )
        )
    );

    code_t *code = compile(program, 2);

    run(code);

    return 0;

}
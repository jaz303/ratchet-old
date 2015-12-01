#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

enum {
    OP_PRINT    = (1 << 26),
    OP_HALT     = (2 << 26)
};

enum {
    AST_STMTS,
    AST_PRINT
};

enum {
    T_NIL,
    T_AST,
    T_INT
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

#define MK_AST1(type, n1) mk_ast1(type, n1)
#define MK_AST2(type, n1, n2) mk_ast2(type, n1, n2)

typedef uint32_t inst_t;

typedef struct {
    val_t *constants;
    inst_t *code;
} code_t;

code_t* compile(val_t program) {
    code_t *co = malloc(sizeof(code_t));
    co->constants = malloc(sizeof(val_t) * 128);
    co->code = malloc(sizeof(inst_t) * 128);

    int k_ix = 0;
    int inst_ix = 0;

    int this_node = ast_pool[program.ival].o1.ival;

    while (this_node) {
        switch (ast_pool[this_node].type) {
            case AST_PRINT:
                co->constants[k_ix] = ast_pool[this_node].o1;
                co->code[inst_ix++] = OP_PRINT | k_ix;
                k_ix++;
                break;
            default:
                printf("unknown node type: %d\n", ast_pool[this_node].type);
                return 0;
        } 
        this_node = ast_pool[this_node].o2.ival;
    }

    co->code[inst_ix] = OP_HALT;

    return co;
}

void run(code_t *co) {
    int ip = 0;
    while (1) {
        inst_t op = co->code[ip++];
        switch (op & 0xfc000000) {
            case OP_PRINT:
                printf("print: %d\n", co->constants[op & 0x03FFFFFF].ival);
                break;
            case OP_HALT:
                printf("execution terminated\n");
                return;
        } 
    }
}

int main(int argc, char *argv[]) {

    val_t program = MK_AST1(AST_STMTS,
        MK_AST2(AST_PRINT,
            mk_int(100),
            MK_AST2(AST_PRINT,
                mk_int(25),
                MK_AST2(AST_PRINT,
                    mk_int(50),
                    mk_nil()
                )
            )
        )
    );

    code_t *code = compile(program);

    run(code);

    return 0;

}
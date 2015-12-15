// AST node type tags
// The actual AST structs are declared in ast.inc.cpp
enum {
    AST_BIN_OP, AST_CALL, AST_FN_DEF, AST_IDENT, AST_LIST, AST_PRINT, AST_UN_OP, AST_WHILE
};

enum opcode_t {
    OP_PRINT    = (1 << 26),
    OP_HALT     = (2 << 26),
    OP_ADD      = (3 << 26),
    OP_SUB      = (4 << 26),
    OP_MUL      = (5 << 26),
    OP_DIV      = (6 << 26),
    OP_LOADK    = (7 << 26),
    OP_COPY     = (8 << 26),
    OP_CALL     = (9 << 26),
    OP_LT       = (10 << 26),
    OP_JMP      = (11 << 26),
    OP_JMPF     = (12 << 26)
};

// Mask that identifies an operator_t as a simple binary operator;
// that is, the operator maps directly to a VM opcode.
const int OPERATOR_SIMPLE_BINOP_MASK = 0x80;

enum operator_t {
    OPERATOR_NONE       = 0x00,

    OPERATOR_ASSIGN     = 0x01,

    OPERATOR_ADD        = OPERATOR_SIMPLE_BINOP_MASK | 0,
    OPERATOR_SUB        = OPERATOR_SIMPLE_BINOP_MASK | 1,
    OPERATOR_MUL        = OPERATOR_SIMPLE_BINOP_MASK | 2,
    OPERATOR_DIV        = OPERATOR_SIMPLE_BINOP_MASK | 3,
    OPERATOR_LT         = OPERATOR_SIMPLE_BINOP_MASK | 4
};

// Must be in the same order as the corresponding operator_ts, above.
const opcode_t rt_simple_binop_opcodes[] = {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_LT
};

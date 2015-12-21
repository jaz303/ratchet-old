// AST node type tags
// The actual AST structs are declared in ast.inc.cpp
enum {
    AST_BIN_OP, AST_CALL, AST_FN_DEF, AST_IDENT, AST_IF,
    AST_LIST, AST_PRINT, AST_UN_OP, AST_WHILE
};

#define OP_SHIFT 26
#define OP_BITS(x) (x << OP_SHIFT)

enum opcode_t {
    OP_PRINT    = OP_BITS(1),
    OP_HALT     = OP_BITS(2),
    OP_ADD      = OP_BITS(3),
    OP_SUB      = OP_BITS(4),
    OP_MUL      = OP_BITS(5),
    OP_DIV      = OP_BITS(6),
    OP_LOADK    = OP_BITS(7),
    OP_COPY     = OP_BITS(8),
    OP_CALL     = OP_BITS(9),
    OP_LT       = OP_BITS(10),
    OP_LE       = OP_BITS(11),
    OP_GT       = OP_BITS(12),
    OP_GE       = OP_BITS(13),
    OP_EQ       = OP_BITS(12),
    OP_NEQ      = OP_BITS(13),
    OP_JMP      = OP_BITS(11),
    OP_JMPF     = OP_BITS(12)
};

// Mask that identifies an operator_t as a simple binary operator;
// that is, the operator maps directly to a VM opcode.
const int OPERATOR_SIMPLE_BINOP_MASK = 0x80;
const int OPERATOR_SIMPLE_UNOP_MASK = 0x40;

enum operator_t {
    OPERATOR_NONE       = 0x00,

    OPERATOR_ASSIGN     = 0x01,

    OPERATOR_ADD        = OPERATOR_SIMPLE_BINOP_MASK | 0,
    OPERATOR_SUB        = OPERATOR_SIMPLE_BINOP_MASK | 1,
    OPERATOR_MUL        = OPERATOR_SIMPLE_BINOP_MASK | 2,
    OPERATOR_POW        = OPERATOR_SIMPLE_BINOP_MASK | 3,
    OPERATOR_DIV        = OPERATOR_SIMPLE_BINOP_MASK | 4,
    OPERATOR_LT         = OPERATOR_SIMPLE_BINOP_MASK | 5,
    OPERATOR_LE         = OPERATOR_SIMPLE_BINOP_MASK | 6,
    OPERATOR_GT         = OPERATOR_SIMPLE_BINOP_MASK | 7,
    OPERATOR_GE         = OPERATOR_SIMPLE_BINOP_MASK | 8,
    OPERATOR_EQ         = OPERATOR_SIMPLE_BINOP_MASK | 9,
    OPERATOR_NEQ        = OPERATOR_SIMPLE_BINOP_MASK | 10,

    OPERATOR_UNMINUS    = OPERATOR_SIMPLE_UNOP_MASK | 0,
    OPERATOR_UNPLUS     = OPERATOR_SIMPLE_UNOP_MASK | 1,
    OPERATOR_NEGATE     = OPERATOR_SIMPLE_UNOP_MASK | 2
};

// Must be in the same order as the corresponding operator_ts, above.
const opcode_t rt_simple_binop_opcodes[] = {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_EQ,
    OP_NEQ
};

const opcode_t rt_simple_unop_opcodes[] = {

};

typedef struct {
    int length;
    char str[0];
} rt_string_t;
/*
 *  token           prefix              prefix              infix   infix   infix               infix
 *                  parser              operator            prec.   rassoc. parser              operator
 */
OP( TOK_LPAREN,     parse_paren_exp,    OPERATOR_NONE,      32,     -1,     parse_call,         OPERATOR_NONE   ), \

OP( TOK_TWOSTAR,    NULL,               OPERATOR_NONE,      31,     0,      parse_infix_op,     OPERATOR_POW    ), \

OP( TOK_STAR,       NULL,               OPERATOR_NONE,      30,     0,      parse_infix_op,     OPERATOR_MUL    ), \
OP( TOK_SLASH,      NULL,               OPERATOR_NONE,      30,     0,      parse_infix_op,     OPERATOR_DIV    ), \

OP( TOK_PLUS,       parse_prefix_op,    OPERATOR_UNPLUS,    29,     0,      parse_infix_op,     OPERATOR_ADD    ), \
OP( TOK_SUB,        parse_prefix_op,    OPERATOR_UNMINUS,   29,     0,      parse_infix_op,     OPERATOR_SUB    ), \

OP( TOK_LT,         NULL,               OPERATOR_NONE,      16,     0,      parse_infix_op,     OPERATOR_LT     ), \
OP( TOK_LE,         NULL,               OPERATOR_NONE,      16,     0,      parse_infix_op,     OPERATOR_LE     ), \
OP( TOK_GT,         NULL,               OPERATOR_NONE,      16,     0,      parse_infix_op,     OPERATOR_GT     ), \
OP( TOK_GE,         NULL,               OPERATOR_NONE,      16,     0,      parse_infix_op,     OPERATOR_GE     ), \
OP( TOK_EQ,         NULL,               OPERATOR_NONE,      15,     0,      parse_infix_op,     OPERATOR_EQ     ), \
OP( TOK_NEQ,        NULL,               OPERATOR_NONE,      15,     0,      parse_infix_op,     OPERATOR_NEQ    ), \

OP( TOK_ASSIGN,     NULL,               OPERATOR_NONE,      1,      1,      parse_infix_op,     OPERATOR_ASSIGN ), \







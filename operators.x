/*
 *  token           prefix              infix   infix   infix               infix
 *                  parser              prec.   rassoc. parser              operator
 */
OP( TOK_LPAREN,     parse_paren_exp,    10,     -1,     parse_call,         OPERATOR_NONE   ), \
OP( TOK_ASSIGN,     NULL,               1,      0,      parse_infix_op,     OPERATOR_ASSIGN ), \
OP( TOK_PLUS,       parse_prefix_op,    2,      0,      parse_infix_op,     OPERATOR_ADD    ), \
OP( TOK_SUB,        parse_prefix_op,    2,      0,      parse_infix_op,     OPERATOR_SUB    ), \
OP( TOK_STAR,       NULL,               3,      0,      parse_infix_op,     OPERATOR_MUL    ), \
OP( TOK_SLASH,      NULL,               3,      0,      parse_infix_op,     OPERATOR_DIV    ), \
OP( TOK_LT,         NULL,               3,      0,      parse_infix_op,     OPERATOR_LT     ), \

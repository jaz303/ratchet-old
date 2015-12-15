/*
 *  token           prefix              prefix 				infix   infix   infix               infix
 *                  parser              operator 			prec.   rassoc. parser              operator
 */
OP( TOK_LPAREN,     parse_paren_exp,    OPERATOR_NONE,		10,     -1,     parse_call,         OPERATOR_NONE   ), \
OP( TOK_ASSIGN,     NULL,               OPERATOR_NONE,		1,      0,      parse_infix_op,     OPERATOR_ASSIGN ), \
OP( TOK_PLUS,       parse_prefix_op,    OPERATOR_UNPLUS,	2,      0,      parse_infix_op,     OPERATOR_ADD    ), \
OP( TOK_SUB,        parse_prefix_op,    OPERATOR_UNMINUS,   0,      0,		parse_infix_op,     OPERATOR_SUB    ), \
OP( TOK_STAR,       NULL,               OPERATOR_NONE,      0,      0, 		parse_infix_op,     OPERATOR_MUL    ), \
OP( TOK_SLASH,      NULL,               OPERATOR_NONE,      0,      0,		parse_infix_op,     OPERATOR_DIV    ), \
OP( TOK_LT,         NULL,               OPERATOR_NONE,      0,      0,		parse_infix_op,     OPERATOR_LT     ), \

/*
 *  token           prefix              infix   infix   infix
 *                  parser              prec.   rassoc. parser
 */
OP( TOK_LPAREN,     parse_paren_exp,    10,     -1,     parse_call      ), \
OP( TOK_ASSIGN,     NULL,               1,      0,      parse_infix_op  ), \
OP( TOK_PLUS,       parse_prefix_op,    2,      0,      parse_infix_op  ), \
OP( TOK_SUB,        parse_prefix_op,    2,      0,      parse_infix_op  ), \
OP( TOK_LT,         NULL,               3,      0,      parse_infix_op  ), \

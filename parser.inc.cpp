typedef struct rt_parser {
	rt_lexer_t lexer;
	int curr;
	const char *error;
} rt_parser_t;

typedef val_t (*prefix_parse_f)(rt_parser_t *p);
typedef val_t (*infix_parse_f)(rt_parser_t *p, val_t left);

val_t parse_prefix_op(rt_parser_t*);
val_t parse_paren_exp(rt_parser_t*);
val_t parse_infix_op(rt_parser_t*, val_t);
val_t parse_call(rt_parser_t*, val_t);
val_t parse_statements(rt_parser_t*, int);
val_t parse_expression(rt_parser_t*, int);

prefix_parse_f prefix_parsers[] = {
	NULL,
	#define OP(_1, prefix_parse, _2, _3, _4) prefix_parse
	#include "operators.x"
	#undef OP
};

struct infix_op {
	int precedence;
	int right_associative;
	infix_parse_f parser;
} infix_ops[] = {
	{ -1, -1, NULL },
	#define OP(_1, _2, infix_prec, infix_rassoc, infix_parse) \
		{ infix_prec, infix_rassoc, infix_parse }
	#include "operators.x"
	#undef OP
};

#define SKIP_NL() \
	while (AT(TOK_NL)) NEXT()

#define PARSE_INTO(var, rule, ...) \
	var = parse_##rule(p, ## __VA_ARGS__); \
	if (p->error) return mk_nil()

#define PARSE(var, rule, ...) \
	val_t var; \
	PARSE_INTO(var, rule, ## __VA_ARGS__)

#define PARSE_STATEMENTS(var, term) \
	val_t var = parse_statements(p, term); \
	if (p->error) return mk_nil()

#define PARSE_STATEMENT(var, term) \
	val_t var = parse_statement(p, term); \
	if (p->error) return mk_nil()

#define ERROR(msg) \
	printf("error: %s\n", msg); \
	p->error = msg; \
	return mk_nil()

#define ACCEPT(tok) \
	if (!AT(tok)) { ERROR("expected: " #tok); } \
	NEXT()

#define NEXT() \
	p->curr = rt_lexer_next(&p->lexer)

#define CURR() \
	(p->curr)

#define TEXT() \
	(p->lexer.tok)

#define TEXT_LEN() \
	(p->lexer.tok_len)

#define AT(tok) \
	(CURR() == tok)

#define MK2(type, arg1, arg2) \
	mk_ast_##type(arg1, arg2)

val_t parse_call(rt_parser_t *p, val_t left) {
	ACCEPT(TOK_LPAREN);
	// TODO: parse argument list
	ACCEPT(TOK_RPAREN);
	return mk_ast_call(left, mk_nil());
}

val_t parse_ident(rt_parser_t *p) {
	int sym = rt_intern(TEXT(), TEXT_LEN());
	ACCEPT(TOK_IDENT);
	return mk_ident(sym);
}

val_t parse_int(rt_parser_t *p) {
	// TODO: overflow
	// TODO: use correct int type
	int val = 0;
	for (int i = 0; i < p->lexer.tok_len; ++i) {
		val = (val * 10) + (p->lexer.tok[i] - '0');
	}
	NEXT();
	return mk_int(val);
}

val_t parse_prefix_op(rt_parser_t *p) {
	// TODO: this should consume the token,
	// create the unary operator, then call back
	// into parse_expression()
	fprintf(stderr, "unary expressions not supported\n");
	ERROR("tmp: unary");
}

val_t parse_paren_exp(rt_parser_t *p) {
	ACCEPT(TOK_LPAREN);
	PARSE(exp, expression, 0);
	ACCEPT(TOK_RPAREN);
	return exp;
}

val_t parse_infix_op(rt_parser_t *p, val_t left) {
	int optok = CURR();
	int next_precedence = infix_ops[optok].precedence
							- (infix_ops[optok].right_associative ? 1 : 0);
	if (next_precedence < 0) {
		ERROR("illegal precedence value; this is a bug.");
	}
	NEXT();
	PARSE(right, expression, next_precedence);
	return mk_ast_binop(optok, left, right);
}

val_t parse_expression(rt_parser_t *p, int precedence) {
	val_t left;

	if (AT(TOK_IDENT)) {
		PARSE_INTO(left, ident);
	} else if (AT(TOK_INT)) {
		PARSE_INTO(left, int);
	} else if ((CURR() < TOK_OP_MAX)
				&& (prefix_parsers[CURR()] != NULL)) {
		left = prefix_parsers[CURR()](p);
		if (p->error) return mk_nil();
	} else {
		// TODO: better error message
		ERROR("parse error");
	}

	while ((CURR() < TOK_OP_MAX)
			&& (infix_ops[CURR()].parser != NULL)
			&& (precedence < infix_ops[CURR()].precedence)) {
		left = infix_ops[CURR()].parser(p, left);
		if (p->error) return mk_nil();
	}

	return left;
}

val_t parse_block(rt_parser_t *p) {
	ACCEPT(TOK_LBRACE);
	SKIP_NL();
	PARSE_STATEMENTS(stmts, TOK_RBRACE);
	ACCEPT(TOK_RBRACE);
	SKIP_NL();
	return stmts;
}

val_t parse_while(rt_parser_t *p) {
	ACCEPT(TOK_WHILE);
	PARSE(cond, expression, 0);
	SKIP_NL();
	PARSE(stmts, block);
	return MK2(while, cond, stmts);
}

val_t parse_statement(rt_parser_t *p, int terminator) {
	if (AT(TOK_WHILE)) {
		return parse_while(p);
	} else {
		PARSE(exp, expression, 0);
		if (AT(TOK_NL)) {
			SKIP_NL();
		} else if (AT(terminator)) {
			// do nothing
		} else {
			ERROR("expected: newline or terminator");
		}
		return exp;
	}
}

val_t parse_statements(rt_parser_t *p, int terminator) {
	val_t head = mk_nil();
	val_t tail = mk_nil();
	while (!AT(terminator)) {
		PARSE_STATEMENT(stmt, terminator);
		val_t node = mk_ast_list(stmt, mk_nil());
		if (nil_p(head)) {
			head = tail = node;
		} else {
			((ast_list_t*)tail.ast)->next = node;
			tail = node;
		}
	}
	return head;
}

val_t parse_module(rt_parser_t *p) {
	SKIP_NL();
	PARSE_STATEMENTS(stmts, TOK_EOF);
	ACCEPT(TOK_EOF);
	return stmts;
}

/* Public Interface */

void rt_parser_init(rt_parser_t *parser) {
	parser->curr = rt_lexer_next(&parser->lexer);
	parser->error = NULL;
}

val_t rt_parse_module(rt_parser_t *parser) {
	return parse_module(parser);
}

#undef SKIP_NL
#undef PARSE_INTO
#undef PARSE
#undef PARSE_STATEMENTS
#undef PARSE_STATEMENT
#undef ERROR
#undef ACCEPT
#undef NEXT
#undef CURR
#undef TEXT
#undef TEXT_LEN
#undef AT
#undef MK2
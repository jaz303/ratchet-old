typedef struct rt_parser {
	rt_lexer_t lexer;
	int curr;
	char *error;
} rt_parser_t;

val_t parse_statements(rt_parser_t*, int);

#define SKIP_NL() \
	while (AT(TOK_NL)) NEXT()

#define PARSE(var, rule) \
	val_t var = parse_##rule(p); \
	if (p->error) return mk_nil()

#define PARSE_STATEMENTS(var, term) \
	val_t var = parse_statements(p, term); \
	if (p->error) return mk_nil()

#define ERROR(msg) \
	p->error = msg; \
	return mk_nil()

#define ACCEPT(tok) \
	if (!AT(tok)) ERROR("expected: " #tok); \
	NEXT()

#define NEXT() \
	p->curr = rt_lexer_next(&p->lexer)

#define CURR() \
	(p->curr)

#define AT(tok) \
	(p->curr == tok)

#define MK2(type, arg1, arg2) \
	mk_ast_##type(arg1, arg2)

val_t parse_expression(rt_parser_t *p) {
	return mk_nil();
}

val_t parse_primary(rt_parser_t *p) {
	if (AT(TOK_INT)) {
		// TODO: overflow
		// TODO: use correct int type
		int val = 0;
		for (int i = 0; i < p->lexer.tok_len; ++i) {
			val = (val * 10) + (p->lexer.tok[i] - '0');
		}
		ACCEPT(TOK_INT);
		return mk_int(val);
	} else if (AT(TOK_IDENT)) {
		int sym = rt_intern(p->lexer.tok, p->lexer.tok_len);
		ACCEPT(TOK_IDENT);
		return mk_ident(sym);
	} else if (AT(TOK_LPAREN)) {
		ACCEPT(TOK_LPAREN);
		PARSE(exp, expression);
		ACCEPT(TOK_RPAREN);
		return exp;
	} else {
		ERROR("invalid primary");
	}
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
	PARSE(cond, expression);
	SKIP_NL();
	PARSE(stmts, block);
	return MK2(while, cond, stmts);
}

val_t parse_statement(rt_parser_t *p) {
	if (AT(TOK_WHILE)) {
		return parse_while(p);
	} else {
		PARSE(exp, expression);
		if (AT(TOK_NL)) {
			SKIP_NL();
		} else if (AT(TOK_RBRACE) || AT(TOK_EOF)) {
			// do nothing
		} else {
			ERROR("expected: newline or EOF");
		}
		return exp;
	}
}

val_t parse_statements(rt_parser_t *p, int terminator) {
	while (!AT(terminator)) {
		PARSE(stmt, statement);
		// concat
	}
	return mk_nil(); // TODO Fix
}

val_t parse_module(rt_parser_t *p) {
	SKIP_NL();
	PARSE_STATEMENTS(stmts, TOK_EOF);
	ACCEPT(TOK_EOF);
	return stmts;
}

//
// Public interface

void rt_parser_init(rt_parser_t *parser) {
	parser->curr = rt_lexer_next(&parser->lexer);
	parser->error = NULL;
}

val_t rt_parse_module(rt_parser_t *parser) {
	return parse_module(parser);
}

#undef SKIP_NL
#undef PARSE
#undef PARSE_STATEMENTS
#undef ERROR
#undef ACCEPT
#undef NEXT
#undef CURR
#undef AT
typedef struct rt_parser {
	rt_lexer_t lexer;
	int curr;
	char *error;
} rt_parser_t;

val_t parse_statements(rt_parser_t*, int);
val_t parse_expression(rt_parser_t *p);

#define SKIP_NL() \
	while (AT(TOK_NL)) NEXT()

#define PARSE(var, rule) \
	val_t var = parse_##rule(p); \
	if (p->error) return mk_nil()

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
		int sym = rt_intern(TEXT(), TEXT_LEN());
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

val_t parse_call(rt_parser_t *p) {
	if (!AT(TOK_IDENT)) {
		ERROR("expected: ident");
	}
	val_t ident = parse_primary(p);
	ACCEPT(TOK_LPAREN);
	ACCEPT(TOK_RPAREN);
	return mk_ast_call(ident, mk_nil());
}

val_t parse_expression(rt_parser_t *p) {
	if (AT(TOK_IDENT)) {
		return parse_call(p);
	} else {
		return parse_primary(p);
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
	printf("curr = %d\n", CURR());
	ACCEPT(TOK_WHILE);
	printf("got while\n");
	PARSE(cond, expression);
	SKIP_NL();
	PARSE(stmts, block);
	return MK2(while, cond, stmts);
}

val_t parse_statement(rt_parser_t *p, int terminator) {
	if (AT(TOK_WHILE)) {
		printf("parsing while...\n");
		return parse_while(p);
	} else {
		PARSE(exp, expression);
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
		printf("parsing statement...\n");
		PARSE_STATEMENT(stmt, terminator);
		printf("statement parsed!\n");
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
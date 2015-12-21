typedef struct rt_parser {
	rt_lexer_t lexer;
	int curr;
	const char *error;
} rt_parser_t;

int pdebug_depth = 0;
void pdebug_print(const char *msg) {
	if (msg[0] == '<') pdebug_depth--;
	for (int i = 0; i < pdebug_depth; ++i) {
		fputs("  ", stderr);
	}
	fputs(msg, stderr);
	fputc('\n', stderr);
	if (msg[0] == '>') pdebug_depth++;
}

#define PDEBUG(msg) pdebug_print(msg)

typedef val_t (*prefix_parse_f)(rt_parser_t *p);
typedef val_t (*infix_parse_f)(rt_parser_t *p, val_t left);

val_t parse_prefix_op(rt_parser_t*);
val_t parse_paren_exp(rt_parser_t*);
val_t parse_infix_op(rt_parser_t*, val_t);
val_t parse_call(rt_parser_t*, val_t);
val_t parse_statements(rt_parser_t*, int);
val_t parse_expression(rt_parser_t*, int);

struct prefix_op {
	prefix_parse_f parser;
	operator_t op;
} prefix_ops[] = {
	{ NULL, OPERATOR_NONE },
	#define OP(_1, prefix_parse, prefix_operator, _2, _3, _4, _5) \
		{ prefix_parse, prefix_operator }
	#include "operators.x"
	#undef OP
};

struct infix_op {
	int precedence;
	int right_associative;
	infix_parse_f parser;
	operator_t op;
} infix_ops[] = {
	{ -1, -1, NULL, OPERATOR_NONE },
	#define OP(_1, _2, _3, infix_prec, infix_rassoc, infix_parse, infix_operator) \
		{ infix_prec, infix_rassoc, infix_parse, infix_operator }
	#include "operators.x"
	#undef OP
};

#define SKIP_NL() \
	while (AT(TOK_NL)) { NEXT(); }

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

val_t parse_expression_list(rt_parser_t *p) {
	PDEBUG("> expression list");
	val_t head = mk_nil(), tail = mk_nil();
	while (1) {
		PARSE(exp, expression, 0);
		val_t node = mk_ast_list(exp, mk_nil());
		if (nil_p(head)) {
			head = tail = node;
		} else {
			((ast_list_t*)tail.ast)->next = node;
			tail = node;
		}
		if (AT(TOK_COMMA)) {
			NEXT();
		} else {
			break;
		}
	}
	PDEBUG("< expression list");
	return head;
}

val_t parse_call(rt_parser_t *p, val_t left) {
	PDEBUG("> call");
	ACCEPT(TOK_LPAREN);
	val_t args;
	if (AT(TOK_RPAREN)) {
		args = mk_nil();
	} else {
		PARSE_INTO(args, expression_list);
	}
	ACCEPT(TOK_RPAREN);
	PDEBUG("< call");
	return mk_ast_call(left, args);
}

val_t parse_ident(rt_parser_t *p) {
	int sym;
	if (AT(TOK_IDENT)) {
		sym = rt_intern(TEXT(), TEXT_LEN());
	}
	ACCEPT(TOK_IDENT);
	PDEBUG("- ident");
	return mk_ident(sym);
}

val_t parse_string(rt_parser_t *p) {
	val_t str = mk_string_from_token(p->lexer.tok, p->lexer.tok_len);
	NEXT();
	return str;
}

val_t parse_int(rt_parser_t *p) {
	// TODO: overflow
	// TODO: use correct int type
	int val = 0;
	for (int i = 0; i < p->lexer.tok_len; ++i) {
		val = (val * 10) + (p->lexer.tok[i] - '0');
	}
	NEXT();
	PDEBUG("- int");
	return mk_int(val);
}

val_t parse_prefix_op(rt_parser_t *p) {
	PDEBUG("> prefix op");
	int optok = CURR();
	NEXT();
	PARSE(exp, expression, 0);
	PDEBUG("< prefix op");
	return mk_ast_unop(prefix_ops[optok].op, exp);
}

val_t parse_paren_exp(rt_parser_t *p) {
	PDEBUG("> paren exp");
	ACCEPT(TOK_LPAREN);
	PARSE(exp, expression, 0);
	ACCEPT(TOK_RPAREN);
	PDEBUG("< paren exp");
	return exp;
}

val_t parse_infix_op(rt_parser_t *p, val_t left) {
	PDEBUG("> infix op");
	int optok = CURR();
	int next_precedence = infix_ops[optok].precedence
							- (infix_ops[optok].right_associative ? 1 : 0);
	if (next_precedence < 0) {
		ERROR("illegal precedence value; this is a bug.");
	}
	NEXT();
	PARSE(right, expression, next_precedence);
	PDEBUG("< infix op");
	return mk_ast_binop(infix_ops[optok].op, left, right);
}

val_t parse_expression(rt_parser_t *p, int precedence) {
	PDEBUG("> expression");

	val_t left;
	if (AT(TOK_IDENT)) {
		PARSE_INTO(left, ident);
	} else if (AT(TOK_TRUE)) {
		left = mk_true();
		NEXT();
	} else if (AT(TOK_FALSE)) {
		left = mk_false();
		NEXT();
	} else if (AT(TOK_STRING)) {
		PARSE_INTO(left, string);
	} else if (AT(TOK_INT)) {
		PARSE_INTO(left, int);
	} else if ((CURR() < TOK_OP_MAX)
				&& (prefix_ops[CURR()].parser != NULL)) {
		left = prefix_ops[CURR()].parser(p);
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

	PDEBUG("< expression");
	return left;
}

val_t parse_block(rt_parser_t *p) {
	PDEBUG("> block");
	ACCEPT(TOK_LBRACE);
	SKIP_NL();
	PARSE_STATEMENTS(stmts, TOK_RBRACE);
	ACCEPT(TOK_RBRACE);
	SKIP_NL();
	PDEBUG("< block");
	return stmts;
}

val_t parse_while(rt_parser_t *p) {
	PDEBUG("> while");
	ACCEPT(TOK_WHILE);
	PARSE(cond, expression, 0);
	SKIP_NL();
	PARSE(stmts, block);
	PDEBUG("< while");
	return MK2(while, cond, stmts);
}

val_t parse_if(rt_parser_t *p) {
	PDEBUG("> if");
	ACCEPT(TOK_IF);
	PARSE(cond, expression, 0);
	SKIP_NL();
	PARSE(stmts, block);
	val_t head = mk_ast_if(cond, stmts);
	val_t tail = head;
	while (AT(TOK_ELSE)) {
		NEXT();
		if (AT(TOK_IF)) {
			NEXT();
			PARSE(cond, expression, 0);
			SKIP_NL();
			PARSE(stmts, block);
			tail = ast_if_cons(tail, cond, stmts);
		} else {
			SKIP_NL();
			PARSE(stmts, block);
			tail = ast_if_cons(tail, stmts);
			break;
		}
	}
	PDEBUG("< if");
	return head;
}

val_t parse_fn_def(rt_parser_t *p) {
	PDEBUG("> fn-def");
	ACCEPT(TOK_DEF);
	if (!AT(TOK_IDENT)) {
		ERROR("expected: identifier");
	}
	int name = rt_intern(p->lexer.tok, p->lexer.tok_len);
	NEXT();
	val_t params_head = mk_nil(), params_tail = mk_nil();
	if (AT(TOK_LPAREN)) {
		NEXT();
		if (!AT(TOK_RPAREN)) {
			while (1) {
				PARSE(param_name, ident);
				val_t node = mk_ast_list(param_name, mk_nil());
				if (nil_p(params_head)) {
					params_head = params_tail = node;
				} else {
					((ast_list_t*)params_tail.ast)->next = node;
					params_tail = node;
				}
				if (AT(TOK_COMMA)) {
					NEXT();
				} else {
					break;
				}
			}
		}
		ACCEPT(TOK_RPAREN);
	}
	SKIP_NL();
	PARSE(body, block);
	PDEBUG("< fn-def");
	return mk_ast_fn_def(name, params_head, body);
}

val_t parse_statement(rt_parser_t *p, int terminator) {
	PDEBUG("> statement");
	val_t stmt;
	if (AT(TOK_WHILE)) {
		PARSE_INTO(stmt, while);
	} else if (AT(TOK_IF)) {
		PARSE_INTO(stmt, if);
	} else if (AT(TOK_DEF)) {
		PARSE_INTO(stmt, fn_def);
	} else {
		PARSE_INTO(stmt, expression, 0);
		if (AT(TOK_NL)) {
			SKIP_NL();
		} else if (AT(terminator)) {
			// do nothing
		} else {
			ERROR("expected: newline or terminator");
		}
	}
	PDEBUG("< statement");
	return stmt;
}

val_t parse_statements(rt_parser_t *p, int terminator) {
	PDEBUG("> statements");
	val_t head = mk_nil(), tail = mk_nil();
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
	PDEBUG("< statements");
	return head;
}

val_t parse_module(rt_parser_t *p) {
	PDEBUG("> module");
	SKIP_NL();
	PARSE_STATEMENTS(stmts, TOK_EOF);
	ACCEPT(TOK_EOF);
	PDEBUG("< module");
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
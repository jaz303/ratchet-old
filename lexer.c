#include "prelude.inc.c"
#include "util.inc.c"

enum {
    TOK_IDENT = 1,
    TOK_ASSIGN,
    TOK_WHILE,
    TOK_LT,
    TOK_INT,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_PLUS,

    TOK_META = 1000,
    TOK_EOF,
    TOK_ERROR
};

typedef struct rt_lexer {
    char *text;
    size_t len;
    size_t pos;
    int state;
    int tok_start;
    int tok_len;
    char *tok;
    char *error;
} rt_lexer_t;

#define MARK() l->tok_start = l->pos; l->tok = &(l->text[l->pos])
#define END() l->tok_len = l->pos - l->tok_start
#define EMIT(tok) return tok
#define CURR() (l->text[l->pos])
#define NEXT() (l->pos++)
#define LEN() (l->tok_len)
#define TEXTEQ(str) rt_lexer_texteq(l, str)
#define ERROR(msg) \
    l->error = msg; \
    return TOK_ERROR

int space_p(char c) {
    return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}

int digit_p(char c) {
    return c >= '0' && c <= '9';
}

int ident_start_p(char c) {
    return (c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
            || (c == '_');
}

int ident_rest_p(char c) {
    return ident_start_p(c) || digit_p(c);
}

int rt_lexer_texteq(rt_lexer_t *l, const char *text) {
    int p = 0;
    while (p < l->tok_len && text[p]) {
        if (l->tok[p] != text[p]) return 0;
        p++;
    }
    return p == l->tok_len && text[p] == 0;
}

void rt_lexer_init(rt_lexer_t *lexer, char *text) {
    lexer->text = text;
    lexer->pos = 0;
    lexer->tok_start = 0;
    lexer->tok_len = 0;
    lexer->tok = NULL;
    lexer->error = NULL;
}

void rt_lexer_clone(rt_lexer_t *d, const rt_lexer_t *s) {
    d->text = s->text;
    d->pos = s->pos;
    d->tok_start = s->tok_start;
    d->tok_len = s->tok_len;
    d->tok = s->tok;
    d->error = s->error;
}

int rt_lexer_next(rt_lexer_t *l) {
    if (l->error) {
        return TOK_ERROR;
    }
    while (space_p(l->text[l->pos])) {
        l->pos++;
    }
    switch (CURR()) {
        case 0: EMIT(TOK_EOF);
        case '(': NEXT(); EMIT(TOK_LPAREN);
        case ')': NEXT(); EMIT(TOK_RPAREN);
        case '{': NEXT(); EMIT(TOK_LBRACE);
        case '}': NEXT(); EMIT(TOK_RBRACE);
        case '<': NEXT(); EMIT(TOK_LT);
        case '+': NEXT(); EMIT(TOK_PLUS);
        case ':':
            NEXT();
            if (CURR() == '=') {
                NEXT();
                EMIT(TOK_ASSIGN);
            } else {
                ERROR("expected: '='");
            }
        default:
            if (ident_start_p(CURR())) {
                MARK(); NEXT();
                while (ident_rest_p(CURR())) {
                    NEXT();
                }
                END();
                if (TEXTEQ("while")) {
                    return TOK_WHILE;
                } else {
                    return TOK_IDENT;
                }
            } else if (digit_p(CURR())) {
                MARK(); NEXT();
                while (digit_p(CURR())) {
                    NEXT();
                }
                END();
                EMIT(TOK_INT);
            } else {
                ERROR("unexpected character in input");
            }
    }
}

int main(int argc, char *argv[]) {
    char *source = readfile(argv[1]);
    rt_lexer_t lexer;
    rt_lexer_init(&lexer, source);
    int tok;
    while ((tok = rt_lexer_next(&lexer)) < TOK_META) {
        printf("%d\n", tok);
    }
}

#undef MARK
#undef END
#undef EMIT
#undef CURR
#undef NEXT
#undef LEN
#undef TEXTEQ
#undef ERROR
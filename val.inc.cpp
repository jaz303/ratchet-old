enum {
    T_NIL,
    T_TRUE,
    T_FALSE,
    T_AST,
    T_INT,
    T_IDENT,
    T_FOREIGN_FN
};

typedef struct val val_t;

typedef val_t (*foreign_fn_f)(val_t *args, int nargs);

struct val {
    int type;
    union {
        int ival;
        ast_node_t *ast;
        foreign_fn_f fn;
    };
};

val_t mk_nil() {
    val_t out = { .type = T_NIL };
    return out;
}

val_t mk_true() {
    val_t out = { .type = T_TRUE };
    return out;
}

val_t mk_false() {
    val_t out = { .type = T_FALSE };
    return out;
}

val_t mk_int(int val) {
    val_t out;
    out.type = T_INT;
    out.ival = val;
    return out;
}

val_t mk_ident(int id) {
    val_t out;
    out.type = T_IDENT;
    out.ival = id;
    return out;
}

int nil_p(val_t v) {
    return v.type == T_NIL;
}

int truthy_p(val_t v) {
    return (v.type != T_NIL) && (v.type != T_FALSE);
}
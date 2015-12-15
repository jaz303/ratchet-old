struct ast_node {
    int type;
};

typedef struct ast_call {
    ast_node_t base;
    val_t callee;
    val_t args;
} ast_call_t;

typedef struct ast_print {
    ast_node_t base;
    val_t exp;
} ast_print_t;

typedef struct ast_list ast_list_t;

struct ast_list {
    ast_node_t base;
    val_t exp;
    val_t next;
};

typedef struct ast_fn_def {
    ast_node_t base;
    int name;
    val_t params;
    val_t body;
} ast_fn_def_t;

typedef struct ast_while {
    ast_node_t base;
    val_t cond;
    val_t body;
} ast_while_t;

typedef struct ast_unop {
    ast_node_t base;
    operator_t op;
    val_t exp;
} ast_unop_t;

typedef struct ast_binop {
    ast_node_t base;
    operator_t op;
    val_t l;
    val_t r;
} ast_binop_t;

#define ALLOC_AST(struct_type, tag) \
    struct_type *node = (struct_type*)malloc(sizeof(struct_type)); \
    ((ast_node_t*)node)->type = tag; \
    val_t val = { .type = T_AST, .ast = (ast_node_t*) node }

val_t mk_ast_list(val_t stmt, val_t next) {
    ALLOC_AST(ast_list_t, AST_LIST);
    node->exp = stmt;
    node->next = next;
    return val;
}

val_t mk_ast_call(val_t callee, val_t args) {
    ALLOC_AST(ast_call_t, AST_CALL);
    node->callee = callee;
    node->args = args;
    return val;
}

val_t mk_ast_print(val_t exp) {
    ALLOC_AST(ast_print_t, AST_PRINT);
    node->exp = exp;
    return val;
}

val_t mk_ast_unop(operator_t op, val_t exp) {
    ALLOC_AST(ast_unop_t, AST_UN_OP);
    node->op = op;
    node->exp = exp;
    return val;
}

val_t mk_ast_binop(operator_t op, val_t l, val_t r) {
    ALLOC_AST(ast_binop_t, AST_BIN_OP);
    node->op = op;
    node->l = l;
    node->r = r;
    return val;
}

val_t mk_ast_while(val_t cond, val_t body) {
    ALLOC_AST(ast_while_t, AST_WHILE);
    node->cond = cond;
    node->body = body;
    return val;
}

val_t mk_ast_fn_def(int name, val_t params, val_t body) {
    ALLOC_AST(ast_fn_def_t, AST_FN_DEF);
    node->name = name;
    node->params = params;
    node->body = body;
    return val;
}

int ast_type(val_t v) {
    return v.ast->type;
}

int ast_list_len(val_t v) {
    int len = 0;
    while (!nil_p(v)) {
        len++;
        v = ((ast_list_t*)v.ast)->next;
    }
    return len;
}
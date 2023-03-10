#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

// 現在着目しているトークン
static Token *s_token;

// 入力プログラム
static char *s_user_input;

void set_user_input(char *input) { s_user_input = input; }

// ローカル変数
static LVar *locals;

// エラー箇所を報告する
void error_at(const char *loc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - s_user_input;
    fprintf(stderr, "%s\n", s_user_input);
    fprintf(stderr, "%*s", pos, "");  // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

bool peek(const char *op) {
    if (s_token->kind != TK_RESERVED || strlen(op) != (size_t)s_token->len || memcmp(s_token->str, op, s_token->len)) {
        return false;
    }
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(const char *op) {
    if (peek(op)) {
        s_token = s_token->next;
        return true;
    }
    return false;
}

bool consume_kind(const TokenKind kind) {
    if (s_token->kind != kind) {
        return false;
    }
    s_token = s_token->next;
    return true;
}

Token *consume_ident() {
    if (s_token->kind != TK_IDENT) {
        return NULL;
    }
    Token *token_old = s_token;
    s_token = s_token->next;
    return token_old;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(const char *op) {
    if (s_token->kind != TK_RESERVED || strlen(op) != (size_t)s_token->len || memcmp(s_token->str, op, s_token->len)) {
        error_at(s_token->str, "'%s'ではありません", op);
    }
    s_token = s_token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int32_t expect_number() {
    if (s_token->kind != TK_NUM) {
        error_at(s_token->str, "数ではありません");
    }

    int32_t val = s_token->val;
    s_token = s_token->next;
    return val;
}

// char *get_ident_token() {
//     Token *maybe_ident = s_token;
//     Token *t = consume_ident();
//     if (t) {
//         return s_token;
//         // return strndup(maybe_ident->str, maybe_ident->len);
//     }
//     return NULL;
// }

bool at_eof() { return s_token->kind == TK_EOF; }

// 新しいトークンを作成してcurに繋げる
Token *new_token(const TokenKind kind, Token *cur, char *str, const int32_t len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

bool startswith(const char *s1, const char *s2) { return strncmp(s1, s2, strlen(s2)) == 0; }

bool is_ident1(const char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_'; }

bool is_ident2(const char c) { return is_ident1(c) || ('0' <= c && c <= '9'); }

bool is_alnum(const char c) { return is_ident2(c); }

// ポインタを更新したいので**pにしている
bool consume_keyword_token(char **p, Token **cur) {
    char keywords[][6] = {"return", "if", "else", "while", "for"};
    TokenKind keywords_token[] = {TK_RETURN, TK_IF, TK_ELSE, TK_WHILE, TK_FOR};
    int keywords_size[] = {6, 2, 4, 5, 3};
    int num_keywords = sizeof(keywords_size) / sizeof(int);
    for (int i = 0; i < num_keywords; i++) {
        int keyword_len = keywords_size[i];
        if (strncmp(*p, keywords[i], keyword_len) == 0 && !isalnum((*p)[keyword_len])) {
            *cur = new_token(keywords_token[i], *cur, *p, keyword_len);
            *p += keyword_len;
            return true;
        }
    }
    return false;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        if (strchr("+-*/()<>;={},&", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (consume_keyword_token(&p, &cur)) {
            continue;
        }

        if (is_ident1(*p)) {
            char *start = p;
            do {
                p++;
            } while (is_ident2(*p));
            cur = new_token(TK_IDENT, cur, start, p - start);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *start = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - start;
            continue;
        }

        error_at(p, "トークナイズできません");
    }
    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

// 変数を名前で検索する。見つからなかった場合はNULLを返す。
LVar *find_lvar(const Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (strlen(var->name) == (size_t)tok->len && !strncmp(tok->str, var->name, tok->len)) {
            return var;
        }
    }
    return NULL;
}

Node *new_node(const NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node *new_binary(const NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_num(const int32_t val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

Function *program(void);
Function *function(void);
Type *declarator(void);
Type *type_suffix(void);
Node *compound_stmt(void);
Node *stmt(void);
Node *expr(void);
Node *assign(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);

// program = function-definition*
Function *program() {
    Function head = {};
    Function *cur = &head;
    while (!at_eof()) {
        cur = cur->next = function();
    }
    return head.next;
}

static char *get_ident(Token *tok) {
    if (tok->kind != TK_IDENT) {
        error_at(tok->str, "識別子ではありません");
    }
    return strndup(tok->str, tok->len);
}

static LVar *new_lvar(char *name, Type *ty) {
    LVar *var = calloc(1, sizeof(LVar));
    var->name = name;
    var->ty = ty;
    var->next = locals;
    var->offset = locals ? locals->offset + 8 : 8;
    locals = var;
    return var;
}

static void create_param_lvars(Type *param) {
    if (!param) {
        return;
    }

    create_param_lvars(param->next);
    new_lvar(get_ident(param->name), param);
}

// functon-definition = declarator "{" compound-stmt
Function *function(void) {
    Type *ty = declarator();

    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(ty->name);
    create_param_lvars(ty->params);
    fn->params = locals;

    expect("{");
    fn->body = compound_stmt();
    fn->locals = locals;

    return fn;
}

// declarator = ident type-suffix
Type *declarator() {
    Token *maybe_ident = consume_ident();
    if (!maybe_ident) {
        error_at(s_token->str, "識別子ではありません");
    }
    Type *type = type_suffix();
    type->name = maybe_ident;
    return type;
}

// type-suffix = ("(" func-params? ")")?
// func-params = param ("," param)*
// param       = declspec declarator
Type *type_suffix() {
    expect("(");
    Type head = {};
    Type *cur = &head;
    while (!peek(")") && cur) {
        Token *tok = consume_ident();
        Type *ty = copy_type(ty_int);
        ty->name = tok;
        cur = cur->next = ty;
        // copy_typeがついている理由がよくわからないので外す
        // cur = cur->next = copy_type(ty);

        if (!consume(",")) {
            break;
        }
    }
    expect(")");
    Type *ty = func_type(NULL);
    ty->params = head.next;
    return ty;
}

// compound-stmt = stmt* "}"
Node *compound_stmt() {
    Node *node = new_node(ND_BLOCK);

    Node head = {};
    Node *cur = &head;
    while (!peek("}") && cur) {
        cur = cur->next = stmt();
    }
    expect("}");
    node->body = head.next;
    return node;
}

Node *stmt(void) {
    Node *node = NULL;

    if (consume_kind(TK_RETURN)) {
        node = new_binary(ND_RETURN, expr(), NULL);
        expect(";");
    } else if (consume_kind(TK_IF)) {
        node = new_node(ND_IF);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume_kind(TK_ELSE)) {
            node->els = stmt();
        }
    } else if (consume_kind(TK_WHILE)) {
        node = new_node(ND_WHILE);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
    } else if (consume_kind(TK_FOR)) {
        node = new_node(ND_FOR);
        expect("(");
        if (!peek(";")) {
            node->init = expr();
        }
        expect(";");
        if (!peek(";")) {
            node->cond = expr();
        }
        expect(";");
        if (!peek(")")) {
            node->inc = expr();
        }
        expect(")");
        node->then = stmt();
    } else if (consume("{")) {
        node = compound_stmt();
    } else if (consume(";")) {
        return new_node(ND_BLOCK);  // おそらくなんでもよいはず
    } else {
        node = expr();
        expect(";");
    }
    return node;
}

// expr = equality
Node *expr(void) { return assign(); }

Node *assign(void) {
    Node *node = equality();
    if (consume("=")) {
        return new_binary(ND_ASSIGN, node, assign());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality(void) {
    Node *node = relational();

    for (;;) {
        if (consume("==")) {
            node = new_binary(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_binary(ND_NE, node, relational());
        } else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational(void) {
    Node *node = add();

    for (;;) {
        if (consume("<=")) {
            node = new_binary(ND_LE, node, add());
        } else if (consume("<")) {
            node = new_binary(ND_LT, node, add());
        } else if (consume(">=")) {
            node = new_binary(ND_LE, add(), node);
        } else if (consume(">")) {
            node = new_binary(ND_LT, add(), node);
        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add(void) {
    Node *node = mul();

    for (;;) {
        if (consume("+")) {
            node = new_binary(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_binary(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul(void) {
    Node *node = unary();

    for (;;) {
        if (consume("*")) {
            node = new_binary(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_binary(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

// unary = ("+" | "-")? unary | primary
Node *unary(void) {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_binary(ND_SUB, new_num(0), unary());
    }
    if (consume("*")) {
        return new_binary(ND_DEREF, unary(), NULL);
    }
    if (consume("&")) {
        return new_binary(ND_ADDR, unary(), NULL);
    }
    return primary();
}

// primary = num
//         | ident ("(" expr? ","? ")")?
//         | "(" expr ")"
Node *primary(void) {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) {
        Node *node = calloc(1, sizeof(Node));

        if (consume("(")) {
            Node head = {};
            Node *cur = &head;
            while (!peek(")")) {
                cur = cur->next = expr();
                if (!consume(",")) {
                    break;
                }
            }
            expect(")");
            node->kind = ND_FUNCALL;
            node->args = head.next;
            node->symbolname = strndup(tok->str, tok->len);
        } else {
            node->kind = ND_LVAR;
            LVar *lvar = find_lvar(tok);
            node->symbolname = strndup(tok->str, tok->len);
            Type *ty = ty_int;
            ty->name = s_token;
            if (!lvar) {
                lvar = new_lvar(node->symbolname, ty);
            }
            node->offset = lvar->offset;
        }

        return node;
    }

    return new_num(expect_number());
}

Function *parse(Token *token_in) {
    s_token = token_in;
    return program();
}
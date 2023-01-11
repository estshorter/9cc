#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "9cc.h"

// ローカル変数
LVar *locals;

// エラー箇所を報告する
void error_at(const char *loc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");  // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(const char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != (size_t)token->len || memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

Token *consume_ident() {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *token_old = token;
    token = token->next;
    return token_old;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(const char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != (size_t)token->len || memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%s'ではありません", op);
    }
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int32_t expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }

    int32_t val = token->val;
    token = token->next;
    return val;
}

bool at_eof() { return token->kind == TK_EOF; }

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

        if (strchr("+-*/()<>;=", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
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
        if (var->len == tok->len && !strncmp(tok->str, var->name, var->len)) {
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

void program(void);
Node *stmt(void);
Node *expr(void);
Node *assign(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);

Node *code[100];

void program(void) {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

Node *stmt(void) {
    Node *node = expr();
    expect(";");
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
    return primary();
}

// primary = "(" expr ")" | num
Node *primary(void) {
    if (consume("(")) {
        Node *node = expr();
        consume(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;

        LVar *lvar = find_lvar(tok);
        if (!lvar) {
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            lvar->offset = locals ? locals->offset + 8 : 8;
            locals = lvar;
        }
        node->offset = lvar->offset;
        return node;
    }

    return new_num(expect_number());
}
#ifndef _9CC_H
#define _9CC_H

#include <stdint.h>

typedef enum {
    TK_RESERVED,  // 記号
    TK_NUM,       // 整数
    TK_EOF,       // 入力の終わり
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;  // トークンの型
    Token *next;     // 次の入力トーケン
    int32_t val;     // kindがTK_NUMの場合、その数値
    char *str;       // トークン文字列
    size_t len;      // トークン長
};

typedef enum {
    ND_ADD,  // +
    ND_SUB,  // -
    ND_MUL,  // *
    ND_DIV,  // /
    ND_EQ,   // ==
    ND_NE,   // !=
    ND_LT,   // <
    ND_LE,   // <=
    ND_NUM,  // 整数
} NodeKind;

typedef struct Node Node;
// 抽象構文木のノードの型
struct Node {
    NodeKind kind;  // ノードの型
    Node *lhs;      // 左辺
    Node *rhs;      // 右辺
    int32_t val;    // kindがND_NUMの場合のみ使う
};

Token *tokenize(char *p);
Node *expr(void);
void gen(Node *node);

// 現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;
#endif
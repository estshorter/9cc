#ifndef _9CC_H
#define _9CC_H

#include <stdint.h>

typedef enum {
    TK_RESERVED,  // 記号
    TK_IDENT,     // 識別子
    TK_NUM,       // 整数
    TK_RETURN,    // リターンキーワード
    TK_IF,        // if,
    TK_ELSE,      // else,
    TK_WHILE,     // while
    TK_FOR,       // for
    TK_EOF,       // 入力の終わり
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind;  // トークンの型
    Token *next;     // 次の入力トーケン
    int32_t val;     // kindがTK_NUMの場合、その数値
    char *str;       // トークン文字列
    int64_t len;     // トークン長
};

typedef enum {
    ND_ADD,      // +
    ND_SUB,      // -
    ND_MUL,      // *
    ND_DIV,      // /
    ND_EQ,       // ==
    ND_NE,       // !=
    ND_LT,       // <
    ND_LE,       // <=
    ND_NUM,      // 整数
    ND_ASSIGN,   // =
    ND_LVAR,     // ローカル変数
    ND_FUNCALL,  // 巻数
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_WHILE,
    ND_FOR,
    ND_BLOCK,
} NodeKind;

typedef struct Node Node;
// 抽象構文木のノードの型
struct Node {
    NodeKind kind;   // ノードの型
    Node *lhs;       // 左辺
    Node *rhs;       // 右辺
    int32_t val;     // kindがND_NUMの場合のみ使う
    int32_t offset;  // kindがND_LVARの場合のみ使う

    // if
    Node *cond;
    Node *then;
    Node *els;

    // for
    Node *init;
    Node *inc;

    // Block
    Node *body;
    Node *next;

    char *symbolname;
    Node *args;
};
typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
    LVar *next;      // 次の変数かNULL
    char *name;      // 変数の名前
    int64_t len;     // 名前の長さ
    int32_t offset;  // RBPからのオフセット
};

// 関数
typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    Node *body;
    LVar *locals;
    int64_t stack_size;
};

int32_t get_stacksize(void);
void set_user_input(char *input);
Token *tokenize(char *p);
Function *parse(Token *token_in);
void generate_code(Function *Function);

#endif
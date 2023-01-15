#ifndef _9CC_H
#define _9CC_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Type Type;
typedef struct Node Node;
typedef struct Token Token;
typedef struct LVar LVar;

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
    ND_ADDR,   // &
    ND_DEREF,  // *
} NodeKind;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;  // ノードの型
    Type *ty;       // Type, e.g. int or pointer to int

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

    // Function call
    char *symbolname;
    Node *args;

    LVar *lvar;  // Used if kind == ND_VAR
};

// ローカル変数の型
struct LVar {
    LVar *next;      // 次の変数かNULL
    char *name;      // 変数の名前
    int32_t offset;  // RBPからのオフセット
    Type *ty;        // Type
};

// 関数
typedef struct Function Function;
struct Function {
    Function *next;
    char *name;
    Node *body;
    LVar *locals;
    int64_t stack_size;
    LVar *params;
};

typedef enum {
    TY_INT,
    TY_PTR,
    TY_FUNC,
} TypeKind;

struct Type {
    TypeKind kind;

    // Pointer
    Type *base;

    // Declaration
    Token *name;

    // Function type
    Type *return_ty;
    Type *params;
    Type *next;
};

extern Type *ty_int;

void set_user_input(char *input);
Token *tokenize(char *p);
Function *parse(Token *token_in);
void generate_code(Function *Function);

bool is_integer(Type *ty);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
void add_type(Node *node);

#endif
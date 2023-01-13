#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "9cc.h"

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

int count(void) {
    static int i = 1;
    return i++;
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void gen_lval(const Node *node) {
    if (node->kind != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }
    printf("# left val %d{\n", node->offset / 8);
    printf("  mov rax, rbp\n");
    printf("  sub rax, %" PRId32 "\n", node->offset);
    printf("  push rax\n");
    printf("# } left val %d\n", node->offset / 8);
}

void gen(const Node *node) {
    if (node == NULL) {
        error("node is NULL");
    }
    switch (node->kind) {
        case ND_NUM:
            printf("  push %d\n", node->val);
            return;
        case ND_LVAR:
            printf("# local var {\n");
            gen_lval(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            printf("# } local var\n");
            return;
        case ND_ASSIGN:
            printf("# assign {\n");
            gen_lval(node->lhs);
            gen(node->rhs);

            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            printf("# } assign\n");
            return;
        case ND_RETURN:
            printf("# return {\n");
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  mov rsp, rbp\n");
            printf("  pop rbp\n");
            printf("  ret\n");
            printf("# } return\n");
            return;
        case ND_IF: {
            int c = count();
            printf("# if {\n");
            printf("#   cond {\n");
            gen(node->cond);
            printf("#   } cond\n");
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            if (node->els) {
                printf("  je  .Lelse%d\n", c);
                printf("#   then {\n");
                gen(node->then);
                printf("#   } then\n");

                printf("  je  .Lend%d\n", c);
                printf(".Lelse%d:\n", c);
                printf("#   { else\n");
                gen(node->els);
                printf("#  n } else\n");
                printf(".Lend%d:\n", c);
            } else {
                printf("  je  .Lend%d\n", c);
                printf("#   then {\n");
                gen(node->then);
                printf("#   } then\n");
                printf(".Lend%d:\n", c);
            }
            printf("# } if\n");
            return;
        }
        case ND_WHILE: {
            int c = count();
            printf("# while {\n");
            printf(".Lbegin%d:\n", c);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%d\n", c);
            gen(node->then);
            printf("  jmp .Lbegin%d\n", c);
            printf(".Lend%d:\n", c);
            printf("# } while\n");
            return;
        }
        case ND_FOR: {
            int c = count();
            if (node->init) {
                gen(node->init);
            }
            printf(".Lbegin%d:\n", c);
            if (node->cond) {
                gen(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je  .Lend%d\n", c);
            }
            gen(node->then);
            if (node->inc) {
                gen(node->inc);
            }
            printf("  jmp .Lbegin%d\n", c);
            printf(".Lend%d:\n", c);
            return;
        }
        case ND_BLOCK: {
            printf("# block {\n");
            for (Node *n = node->body; n; n = n->next) {
                gen(n);
                if (n->kind != ND_WHILE && n->kind != ND_FOR && n->kind != ND_IF && n->kind != ND_BLOCK) {
                    printf("  pop rax # pop unnecessary item\n");
                }
            }
            printf("# } block\n");
            return;
        }
        default:
            // error("wrong type: %d, @ %s (%d)", node->kind, __FILE__, __LINE__);
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("# == {\n");
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            printf("# } ==\n");
            break;
        case ND_NE:
            printf("# != {\n");
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            printf("# != }\n");
            break;
        case ND_LT:
            printf("# < {\n");
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            printf("# } <\n");
            break;
        case ND_LE:
            printf("# <= {\n");
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            printf("# <= {\n");
            break;
        default:
            error("cannot be reached : %s (%d)", __FILE__, __LINE__);
    }

    printf("  push rax\n");
}
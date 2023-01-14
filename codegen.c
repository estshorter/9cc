#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "9cc.h"

static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int s_depth;
static Function *s_current_fn;

static void push(void) {
    s_depth++;
    printf("  push rax # size: %d\n", s_depth);
}

static void pop(char *arg) {
    s_depth--;
    printf("  pop %s # size: %d\n", arg, s_depth);
}

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
    printf("# left val %s{\n", node->symbolname);
    printf("  mov rax, rbp\n");
    printf("  sub rax, %" PRId32 "\n", node->offset);
    printf("# } left val %s\n", node->symbolname);
}

void gen(const Node *node) {
    if (node == NULL) {
        error("node is NULL");
    }
    switch (node->kind) {
        case ND_NUM:
            printf("  mov rax, %d\n", node->val);
            return;
        case ND_LVAR:
            printf("# local var %s {\n", node->symbolname);
            gen_lval(node);
            printf("  mov rax, [rax]\n");
            printf("# } local var %s\n", node->symbolname);
            return;
        case ND_FUNCALL:
            printf("# func %s {\n", node->symbolname);
            int nargs = 0;
            printf("#   args %s {\n", node->symbolname);
            for (Node *n = node->args; n; n = n->next) {
                printf("#   gen arg id %d {\n", nargs + 1);
                gen(n);
                push();
                nargs++;
                printf("#   } gen arg id %d\n", nargs + 1);
            }
            for (int i = nargs - 1; i >= 0; i--) {
                printf("#   push arg id %d {\n", i + 1);
                pop(argreg[i]);
                printf("#   } push arg id %d\n", i + 1);
            }

            printf("#   } args %s\n", node->symbolname);
            printf("  mov rax, 0\n");
            printf("  call %s\n", node->symbolname);
            printf("# } func %s\n", node->symbolname);
            return;
        case ND_ASSIGN:
            printf("# assign {\n");
            gen_lval(node->lhs);
            push();
            gen(node->rhs);
            pop("rdi");
            printf("  mov [rdi], rax\n");
            printf("# } assign\n");
            return;
        case ND_RETURN:
            printf("# return {\n");
            gen(node->lhs);
            printf("  jmp .L.return.%s\n", s_current_fn->name);
            printf("# } return\n");
            return;
        case ND_IF: {
            int c = count();
            printf("# if {\n");
            printf("#   cond {\n");
            gen(node->cond);
            printf("#   } cond\n");
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
            printf("# for {\n");
            if (node->init) {
                printf("#   init {\n");
                gen(node->init);
                printf("#   } init\n");
            }
            printf(".Lbegin%d:\n", c);
            if (node->cond) {
                printf("#   cond {\n");
                gen(node->cond);
                printf("  cmp rax, 0\n");
                printf("  je  .Lend%d\n", c);
                printf("#   } cond\n");
            }
            printf("#   then {\n");
            gen(node->then);
            printf("#   } then\n");
            if (node->inc) {
                printf("#   inc {\n");
                gen(node->inc);
                printf("#   } inc\n");
            }
            printf("  jmp .Lbegin%d\n", c);
            printf(".Lend%d:\n", c);
            printf("# } for\n");
            return;
        }
        case ND_BLOCK: {
            printf("# block {\n");
            for (Node *n = node->body; n; n = n->next) {
                gen(n);
            }
            printf("# } block\n");
            return;
        }
        default:
            // error("wrong type: %d, @ %s (%d)", node->kind, __FILE__, __LINE__);
    }

    gen(node->lhs);
    push();
    gen(node->rhs);
    printf("  mov rdi, rax\n");
    pop("rax");

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
    return;
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
static int align_to(int n, int align) { return (n + align - 1) / align * align; }

void assign_lvar_offsets(Function *prog) {
    for (Function *fn = prog; fn; fn = fn->next) {
        fn->stack_size = fn->locals ? align_to(fn->locals->offset, 16) : 0;
    }
}

void generate_code(Function *fns) {
    assign_lvar_offsets(fns);

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    for (Function *fn = fns; fn; fn = fn->next) {
        printf(".global %s\n", fn->name);
        printf("%s:\n", fn->name);

        // プロローグ
        printf("# prologue {\n");
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %ld # num_lvar: %ld\n", fn->stack_size, fn->stack_size / 8);
        printf("# } prologue\n");

        s_current_fn = fn;
        for (Node *n = fn->body; n; n = n->next) {
            gen(n);
        }

        // エピローグ
        // 最後の式の結果がRAXに残っているのでそれが返り値になる
        printf("# epilogue {\n");
        printf(".L.return.%s:\n", fn->name);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        printf("# } epilogue\n");

        if (s_depth != 0) {
            fprintf(stderr, "stack depth: got: %d, want: 0\n", s_depth);
            fflush(stdout);
            assert(s_depth == 0);
        }
    }
}
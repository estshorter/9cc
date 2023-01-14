#include <stdbool.h>
#include <stdio.h>

#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません");
        return 1;
    }

    // トークナイズする
    set_user_input(argv[1]);
    Token *token = tokenize(argv[1]);
    Node *code[100];
    parse(token, code);

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    printf("# prologue {\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d # num_lvar: %d\n", get_stacksize(), get_stacksize() / 8);
    printf("# } prologue\n");

    // 先頭の式から順にコード生成
    generate_code((const Node **)code);

    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値になる
    printf("# epilogue {\n");
    printf(".L.return:\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    printf("# } epilogue\n");
    return 0;
}
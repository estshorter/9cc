#include <stdbool.h>
#include <stdio.h>

#include "9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません");
        return 1;
    }

    set_user_input(argv[1]);
    // トークナイズする
    Token *token = tokenize(argv[1]);
    Function *fns = parse(token);

    // 先頭の式から順にコード生成
    generate_code(fns);
    return 0;
}
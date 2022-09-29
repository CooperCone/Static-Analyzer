#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"
#include "lexer.h"

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++) {
        // Open file
        Buffer fileBuff = {0};

        if (0 != openAndReadFileToBuffer(argv[i], &fileBuff)) {
            // There was an error reading the file
            printf("Error: couldn't read file: %s\n", argv[i]);
            continue;
        }

        printf("%s\n", fileBuff.bytes);

        // Lex file
        TokenList tokens = {0};

        if (0 != lexFile(fileBuff, &tokens)) {
            continue;
        }

        printTokens(tokens);

        // Parse file
    }
}

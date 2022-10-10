#include "trie.h"

#include <stdio.h>

size_t trieCharToIdx(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a';
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 26 + 26;
    }
    if (c == '/') {
        return 0 + 26 + 26 + 10;
    }
    return -1;
}

void trie_addString(Trie *trie, char *str) {
    if (*str == '\0') {
        trie->isTerminal = true;
        return;
    }

    size_t idx = trieCharToIdx(*str);
    if (idx == -1) {
        printf("Unknown trie char: %c\n", *str);
        return;
    }

    if (trie->nodes[idx] == NULL)
        trie->nodes[idx] = calloc(1, sizeof(Trie));

    trie_addString(trie->nodes[idx], str + 1);
}

bool trie_match(Trie *trie, char *str) {
    if (trie == NULL)
        return false;

    if (*str == '\0')
        return trie->isTerminal;

    size_t idx = trieCharToIdx(*str);
    if (idx == -1) {
        printf("Error: Invalid trie char: %c\n", *str);
        return false;
    }

    return trie_match(trie->nodes[idx], str + 1);
}

bool trie_matchEarlyTerm(Trie *trie, char *str) {
    if (trie == NULL)
        return false;

    if (trie->isTerminal)
        return true;

    if (*str == '\0')
        return false;

    size_t idx = trieCharToIdx(*str);
    if (idx == -1) {
        printf("Error: Invalid trie char: %c\n", *str);
        return false;
    }

    return trie_matchEarlyTerm(trie->nodes[idx], str + 1);
}

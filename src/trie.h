#pragma once

#include <stdlib.h>
#include <stdbool.h>

// TODO: Get the correct number of nodes including symbols
#define NumTrieNodes (26 + 26 + 10 + 1)

size_t trieCharToIdx(char c);

typedef struct Trie {
    bool isTerminal;
    struct Trie *nodes[NumTrieNodes];
} Trie;

void trie_addString(Trie *trie, char *str);
bool trie_match(Trie *trie, char *str);
bool trie_matchEarlyTerm(Trie *trie, char *str);

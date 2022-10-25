#pragma once

#include <stddef.h>

typedef struct SLNode {
    struct SLNode *next;
} SLNode;

void *slNode_getData(SLNode *node);

typedef struct {
    size_t size;
    SLNode *head;
    SLNode *tail;
} SLList;


void sll_append(SLList *list, void *data, size_t size);

#define sll_appendLocal(list, data) sll_append(list, &data, sizeof(data))
#define sll_foreach(list, name) for (SLNode *name = list.head;\
    name != NULL; name = name->next)

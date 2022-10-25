#include "linkedList.h"

#include <stdlib.h>
#include <string.h>

void *slNode_getData(SLNode *node) {
    return (void*)(node + 1);
}

void sll_append(SLList *list, void *data, size_t size) {
    SLNode *node = calloc(1, sizeof(SLNode) + size);
    memcpy(node + 1, data, size);

    if (list->size == 0) {
        list->head = node;
        list->tail = node;
    }
    else {
        list->tail->next = node;
        list->tail = node;
    }
    
    list->size++;
}

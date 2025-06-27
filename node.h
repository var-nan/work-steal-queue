#ifndef NODES_H
#define NODES_H

#include <cstdint>
#include <cstdlib>

typedef struct node {
    uint8_t padding[56];
    node *next;
}node;

typedef struct {
    node *start;
    node *end;
    size_t size;
} llist;

#endif
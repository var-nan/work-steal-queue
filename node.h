#ifndef NODES_H
#define NODES_H

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <utility>

typedef struct node {
    uint32_t id;
    uint32_t layerNo;
    uint32_t sleeptime;
    std::vector<std::pair<uint32_t, uint32_t>> children; 
    uint8_t padding[4];
    node *next;
}node;

typedef struct {
    node *start;
    node *end;
    size_t size;
} llist;

class work_steal_queue{
public:
    // virtual void push(node *temp) = 0;
    virtual void b_push(const llist& nodes) = 0;

    virtual node *pop() = 0;
    virtual llist b_steal(double proportion) = 0;
};

#endif
#ifndef NODES_H
#define NODES_H

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <utility>
#include <atomic>

// typedef struct node {
//     uint32_t id;
//     uint32_t layerNo;
//     uint32_t sleeptime;
//     std::vector<std::pair<uint32_t, uint32_t>> children; 
//     std::vector<size_t> childs;
//     std::atomic<bool> visited;
//     uint8_t padding[4];
//     node *next;

//     // node() = default;
//     // node(uint32_t id_): id{id}{}
// }node;

class node {
public:
    uint32_t id = 0;
    uint32_t layerNo = 0;
    uint32_t sleeptime = 0;
    std::vector<std::pair<uint32_t, uint32_t>> children;
    std::vector<size_t> childs;
    // std::atomic<bool> visited = false;
    
    node *next = nullptr;

    node() = default;
    node(uint32_t id_) : id{id_}, layerNo{0}{};
};

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
#ifndef CL_Q_H
#define CL_Q_H

#include <vector>
#include <atomic>
#include <thread>
#include <tuple>

#include "node.h"

using namespace std;

typedef node cl_node;

class ChaseLevQueue {
private:
    static constexpr size_t capacity = 1024;
    atomic<size_t> top{0};
    atomic<size_t> bottom{0};
    // cl_node* buffer[capacity];
    std::vector<cl_node *> buffer;
    std::atomic<size_t> cas_counter{0};

public:
    ChaseLevQueue() {
        buffer = vector<cl_node *>(capacity);
        for (size_t i = 0; i < capacity; ++i) buffer[i] = nullptr;
    }

    void push(llist nodes) {
        // cl_node* curr = get<0>(nodes);
        cl_node *curr = nodes.start;
        while (curr) {
            size_t b = bottom.load(memory_order_relaxed);
            buffer[b % capacity] = curr;
            curr = curr->next;
            bottom.store(b + 1, memory_order_release);
        }
    }

    void b_push(llist nodes){
        push(nodes);
    }

    cl_node* pop() {
        size_t b = bottom.load(memory_order_relaxed) - 1;
        bottom.store(b, memory_order_relaxed);
        size_t t = top.load(memory_order_acquire);
        if (t <= b) {
            cl_node* n = buffer[b % capacity];
            if (t != b) {
                return n;
            }
            if (!top.compare_exchange_strong(t, t + 1)) {
                cas_counter++;
                return nullptr;
            }
            bottom.store(t + 1, memory_order_relaxed);
            return n;
        } else {
            bottom.store(t, memory_order_relaxed);
            return nullptr;
        }
    }

    llist steal(double proportion) {
        size_t t = top.load(memory_order_acquire);
        size_t b = bottom.load(memory_order_acquire);
        if (t >= b) return {nullptr, nullptr, 0};

        size_t n = max(size_t(1), size_t((b - t) * proportion));
        vector<cl_node*> stolen;
        for (size_t i = 0; i < n; ++i) {
            size_t ti = t + i;
            cl_node* item = buffer[ti % capacity];
            if (item == nullptr) break;
            stolen.push_back(item);
        }
        if (!top.compare_exchange_strong(t, t + stolen.size())) {
            cas_counter++;
            return {nullptr, nullptr, 0};
        }
        for (size_t i = 0; i + 1 < stolen.size(); ++i) {
            stolen[i]->next = stolen[i + 1];
        }
        if (!stolen.empty()) stolen.back()->next = nullptr;
        return stolen.empty() ? llist{nullptr, nullptr, 0} : llist{stolen.front(), stolen.back(), stolen.size()};
    }
};


#endif
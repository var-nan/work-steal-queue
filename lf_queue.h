#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <iostream>
#include <atomic>
#include <tuple>
#include <cassert>

#include "node.h"

#define _queue_limit_ 32


typedef node lf_node;


class lf_queue {
    std::atomic<size_t> size = 0;
    std::atomic<lf_node *> head = nullptr;

public:
    lf_queue() = default;

    [[nodiscard]] size_t getSize() const noexcept {return size.load(std::memory_order_acquire); }
    [[nodiscard]] bool empty() const noexcept {return size.load(std::memory_order_acquire) == 0; }

    /**
     * Inserts the given nodes to the front of the queue.
     * The function can be called by both the master and the worker.
     */
    [[gnu::noinline]] void push(const llist &nodes) {
        /* At any point of time, only one thread (either master or worker) will call this function.
         * When the master is calling this function, the worker must be in waiting state */
        lf_node *start = nodes.start, *end = nodes.end;
        size_t n = nodes.size;
        end->next = head.load(std::memory_order_relaxed);
        // head = start;
        head.store(start, std::memory_order_release); // could be relaxed.
        size.fetch_add(n, std::memory_order_release); // cannot convince this memory order.
    }


    void b_push(const llist &nodes){
        push(nodes);
    }

    /**
     * Pops the top most node from the queue and returns its pointer. Effectively decrements
     * the queue's size by 1.
     */
    [[gnu::noinline]] lf_node *pop() {

        if (!head) return nullptr;
        lf_node *rv = head.load(std::memory_order_acquire);// can be relaxed.
        head.store(rv->next); // change to release or relaxed later.
        size.fetch_sub(1, std::memory_order_acq_rel);
        rv->next = nullptr;
        return rv;
    }

    /**
     * The function returns nodes from the worker queue.
     * Should be called by the master.
     * @param proportion - proportion of nodes to pop from worker queue.
     * @return llist {start,end, n}
     */
    llist m_pop(double proportion) {

        proportion = 1 - proportion; // % of nodes left in queue after successful completion of this operation.

        size_t sz = size.load(); // memory_order::acquire
        if (sz < _queue_limit_) return {nullptr, nullptr, 0};

        size_t n_skip = static_cast<size_t>(static_cast<double>(sz) * proportion);// number of nodes to skip from head.
        size_t rem = sz - n_skip; // number of nodes popped from the queue from the end.
        size_t k = n_skip;

        lf_node *start = head.load(); // memory_order::acquire should also work.

        while (n_skip && start) {
            start = start->next;
            n_skip--;
        }
        // while (--n_skip && start && start->next) start = start->next;

        if (n_skip || !start) return {nullptr, nullptr, 0};

        size_t ssz = size.load(); // memory order: acquire
        if (ssz <= (sz - (k>>1))) {
            return {nullptr, nullptr, 0}; // nodes are popped very quick, retry.
        }
        if (ssz == sz){

        }

        lf_node *begin = start->next;
        start->next = nullptr;

        size_t count = 0;

        lf_node *end = begin; // skipping for now, since not required.

        if(ssz == sz){ 
            // small optimization, if producer didn't pop any node, no need to traverse till end.
            size.fetch_sub(k);
            return {begin, end, k};
        }
        else {
            while (end) {
                count++;
                if (!end->next) break;
                end = end->next;
            }
            size.fetch_sub(count);
            return {begin, end,count};
        }
    }

    llist b_steal(double proportion){
        return m_pop(proportion);
    }
};


#endif //LOCK_FREE_QUEUE_H
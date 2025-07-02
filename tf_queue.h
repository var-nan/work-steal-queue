#ifndef TF_Q_H
#define TF_Q_H

#include "node.h"

#include <taskflow/taskflow.hpp>
#include <taskflow/core/tsq.hpp>

typedef node tf_node;


class tf_ub_queue : public tf::UnboundedTaskQueue<tf_node *>{
public:
    using tf::UnboundedTaskQueue<tf_node *>::push;

    void b_push(const llist& nodes){
        auto [start, end, n] = nodes;
        for (size_t i =0; (start) && i< n ; start = start->next, i++){
            this->push(start);
        }
    }

    llist b_steal(double proportion){
        size_t npop = this->size() * proportion;

        tf_node *start = new tf_node();
        tf_node *current = start;
        size_t added = 0;
        for (size_t i = 0; i < npop; i++, added++){
            tf_node *node = this->steal();
            if(!node) break;
            node->next = nullptr;
            current->next = node;
            current = current->next; 
        }
        tf_node *begin = start->next;
        delete start;
        return {begin, current, added};
    }
};


constexpr size_t TF_BQ_SIZE = 8;

class tf_bq : public tf::BoundedTaskQueue<tf_node *, TF_BQ_SIZE>{
public:
    using tf::BoundedTaskQueue<tf_node *, TF_BQ_SIZE>::try_push;

    // tf_bq() {
    //     this.res
    // }

    void b_push(const llist& nodes){
        auto [start, end, n] = nodes;
        for (size_t i =0; (start) && i < n; start = start->next, i++)
            this->try_push(start);
    }

    llist b_steal(double proportion){
        size_t npop = this->size() * proportion;

        tf_node *start = new tf_node();
        tf_node *curr = start;
        size_t added = 0;
        for (size_t i = 0; i < npop; i++, added++){
            tf_node *nd = this->steal();
            if(!nd) break;
            nd->next = nullptr;
            curr->next = nd;
            curr = curr->next;
        }
        tf_node *begin = start->next;
        delete start;
        return {begin, curr, added};
    }
};

#endif
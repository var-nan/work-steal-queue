#ifndef TF_Q_H
#define TF_Q_H

#include "node.h"

#include <taskflow/taskflow.hpp>

typedef node tf_node;


class tf_ub_queue : public tf::UnboundedTaskQueue<tf_node *>{
public:
    using tf::UnboundedTaskQueue<tf_node *>::push;

    void b_push(const llist& nodes){
        auto [start, end, n] = nodes;
        for (; start ; start = start->next){
            this->push(start);
        }
    }

    llist b_steal(double proportion){
        size_t npop = this->size() * proportion;

        tf_node *start = new tf_node();
        tf_node *current = start;
        for (size_t i = 0; i < npop; i++){
            tf_node *node = this->steal();
            current->next = node;
            current = current->next; 
        }
        return {start->next, current, npop};
    }
};


class tf_bq : public tf::BoundedTaskQueue<tf_node *>{
public:
    using tf::BoundedTaskQueue<tf_node *>::push;
};

#endif
#include <iostream>
#include <benchmark/benchmark.h>
#include <string>
#include "node.h"
#include "lf_queue.h"
#include "tf_queue.h"

using namespace std;

static llist get_nodes(size_t n) {
    node *start = new node();
    node *current = start;
    for (size_t i = 0; i < n; i++){
        current->next = new node();
        current = current->next;
    }
    return {start->next, current, n};
}

static void BM_Push(benchmark::State& state, work_steal_queue *q, const string& label){

    const size_t n_nodes = state.range(0);
    // initialize queue
    for (auto _ : state){
        // get nodes
        state.PauseTiming();
        llist nodes = get_nodes(n_nodes);
        state.ResumeTiming();

        q->b_push(nodes);

        state.PauseTiming();
        while (auto popped_node = q->pop())
            delete popped_node;
        state.ResumeTiming();
    }
    // state.SetItemsProcessed()
}


static void BM_Steal(benchmark::State& state, work_steal_queue *q, const string& label){

    const size_t init_size = state.range(1);
    double proportion = double(state.range(0))/10;
    
    // initialize the queue.
    llist nodes = get_nodes(init_size);
    q->b_push(nodes);

    volatile size_t p;

    for (auto _ : state){

        llist popped_nodes = q->b_steal(proportion);
        p = popped_nodes.size;

        state.PauseTiming();
        // insert them back to queueq
        q->b_push(popped_nodes);
        state.ResumeTiming();
    }

    while(auto popped_node = q->pop()){
        delete popped_node;
    }
}

static void BM_Pop(benchmark::State& state, work_steal_queue *q, const string& label){
    const size_t init_size = state.range(0);
    
    llist nodes = get_nodes(init_size);
    q->b_push(nodes);

    volatile node *p;

    for (auto _ : state){
        auto popped_node = q->pop();
        p = popped_node;

        state.PauseTiming();
        popped_node->next = nullptr;
        llist pushing_nodes = {popped_node, popped_node, 1};
        q->b_push(pushing_nodes);
        state.ResumeTiming();
    }

    while (auto popped_node = q->pop())
        delete popped_node;
}


static void BM_TF_Pop(benchmark::State& state){
    const size_t init_size = state.range(0);
    tf_ub_queue q; 
    llist nodes = get_nodes(init_size);
    q.b_push(nodes);

    volatile node *p;

    for (auto _ : state){
        auto popped_node = q.pop();
        p = popped_node;

        state.PauseTiming();
        popped_node->next = nullptr;
        llist pushing_nodes = {popped_node, popped_node, 1};
        q.b_push(pushing_nodes);
        state.ResumeTiming();
    }

    while (auto popped_node = q.pop())
        delete popped_node;
}


BENCHMARK_CAPTURE(BM_Push, LF_Queue, new lf_queue(), "My queue")->Args({128});
BENCHMARK_CAPTURE(BM_Push, LF_Queue, new lf_queue(), "My queue")->Args({512});
BENCHMARK_CAPTURE(BM_Push, LF_Queue, new lf_queue(), "My queue")->Args({1024});
BENCHMARK_CAPTURE(BM_Pop, LF_Queue, new lf_queue(), "My Queue")->Args({10000});
BENCHMARK_CAPTURE(BM_Steal, LF_Queue, new lf_queue(), "My Queue")->Args({1, 10000});
BENCHMARK_CAPTURE(BM_Steal, LF_Queue, new lf_queue(), "My Queue")->Args({2, 10000});
BENCHMARK_CAPTURE(BM_Steal, LF_Queue, new lf_queue(), "My Queue")->Args({4, 10000});
BENCHMARK_CAPTURE(BM_Steal, LF_Queue, new lf_queue(), "My Queue")->Args({7, 10000});

BENCHMARK_CAPTURE(BM_Push, TF_UB_Queue, new tf_ub_queue(), "Task flow ub Queue")->Args({128});
BENCHMARK_CAPTURE(BM_Push, TF_UB_Queue, new tf_ub_queue(), "Task flow ub Queue")->Args({512});
BENCHMARK_CAPTURE(BM_Push, TF_UB_Queue, new tf_ub_queue(), "Task flow ub Queue")->Args({1024});
BENCHMARK_CAPTURE(BM_Pop, TF_Queue, new tf_ub_queue(), "Task flow ub Queue")->Args({10000});
BENCHMARK_CAPTURE(BM_Steal, TF_Queue, new tf_ub_queue(), "Taskflow ub queue")->Args({1,10000});
BENCHMARK_CAPTURE(BM_Steal, TF_Queue, new tf_ub_queue(), "Taskflow ub queue")->Args({2,10000});
BENCHMARK_CAPTURE(BM_Steal, TF_Queue, new tf_ub_queue(), "Taskflow ub queue")->Args({4,10000});
BENCHMARK_CAPTURE(BM_Steal, TF_Queue, new tf_ub_queue(), "Taskflow ub queue")->Args({7,10000});

BENCHMARK_CAPTURE(BM_Push, TF_BD_Queue, new tf_bq(), "TaskFlow Bounded Queue")->Args({128});
BENCHMARK_CAPTURE(BM_Push, TF_BD_Queue, new tf_bq(), "TaskFlow Bounded Queue")->Args({512});
BENCHMARK_CAPTURE(BM_Push, TF_BD_Queue, new tf_bq(), "TaskFlow Bounded Queue")->Args({1024});
BENCHMARK_CAPTURE(BM_Pop, TF_BD_Queue, new tf_bq(), "Taskflow Bounded Queue")->Args({10000});
BENCHMARK_CAPTURE(BM_Steal, TF_BD_Queue, new tf_bq(), "Taskflow Bounded queue")->Args({1,10000});
BENCHMARK_CAPTURE(BM_Steal, TF_BD_Queue, new tf_bq(), "Taskflow Bounded queue")->Args({2,10000});
BENCHMARK_CAPTURE(BM_Steal, TF_BD_Queue, new tf_bq(), "Taskflow Bounded queue")->Args({4,10000});
BENCHMARK_CAPTURE(BM_Steal, TF_BD_Queue, new tf_bq(), "Taskflow Bounded queue")->Args({7,10000});

// BENCHMARK(BM_TF_Pop)->Args({10000});

BENCHMARK_MAIN();
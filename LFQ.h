#ifndef _LFQ_H
#define _LFQ_H

#include <iostream>
#include <thread>
#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <tuple>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

#include "node.h"

// #include <benchmark/benchmark.h>

using namespace std;

enum WORKLOAD {
    PUSH_HEAVY = 0,
    POP_HEAVY = 1,
    STEAL_HEAVY = 2
};


template<typename T> 
class TestQ {
    std::string name;
    const size_t iterations = 10;

    llist get_nodes(size_t n){
        node *start = new node();
        node *current = start;
        for (size_t i = 0; i < n; i++){
            current->next = new node();
            current = current->next;
        }
        return {start->next, current, n};
    }

public:
    TestQ(std::string name_): name{name_}{}

    void testPush(size_t init_size, size_t npush){
        
        const size_t iterations = 10;
        double average_time = 0.0;
        double total_sum = 0.0;

        for(size_t iter = 0; iter < iterations; iter++){
            T lfq; // queue should be empty?
            vector<double> push_latencies(npush, 0.0);

            std::thread producer ([&]{
                // push n iterms to the queue.
                for (size_t i = 0; i < npush; i++){

                    node *dummy_node = new node();
                    llist new_list = {dummy_node, dummy_node, 1};

                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();

                    lfq.b_push(new_list);

                    auto end = std::chrono::high_resolution_clock::now();
                    push_latencies[i] = std::chrono::duration<double>(end-start).count();
                    asm volatile("" ::: "memory");
                }
            });
            producer.join();

            // compute average.
            double sum = 0.0;
            for (double t: push_latencies) sum += (t * (1e6));
            double avg = sum/push_latencies.size();
            average_time += avg; 
            total_sum += sum;
        }
        average_time = average_time/iterations;
        total_sum = total_sum/iterations;

        cout << name << " push:\t" << average_time << " ms. Total:\t" << total_sum << " ms." << endl;

        // cout << "myq - push:\t" << average_time <<" ms. Total:\t" << total_sum << " ms." << endl;
    }


    void testPop(size_t init_size, size_t npop){
        const size_t iterations = 10;
        double average_time = 0.0;
        double total_sum = 0.0;

        for(size_t iter = 0; iter < iterations; iter++){
            T lfq; // should be initialized.

            // for (size_t i = 0; i < init_size; i++){
            //     lf_node *temp = new lf_node();
            //     lfq.push({temp, temp, 1});
            // }

            if(init_size){
                llist nodes = get_nodes(init_size);
                lfq.b_push(nodes);
            }

            vector<double> push_latencies(npop, 0.0);

            std::thread consumer ([&]{
                // pop from queue.
                volatile size_t uncached_num;
                for (size_t i = 0; i < npop; i++){

                    node *dummy_node = new node();
                    llist new_list = {dummy_node, dummy_node, 1};

                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();

                    node *popped_node = lfq.pop();

                    auto end = std::chrono::high_resolution_clock::now();
                    push_latencies[i] = std::chrono::duration<double>(end-start).count();  
                    asm volatile("" ::: "memory");
                    uncached_num = popped_node->padding[0];

                    delete popped_node;
                }
            });
            consumer.join();

            // compute average.
            double sum = 0.0;
            for (double t: push_latencies) sum += (t * (1e6));
            double avg = sum/push_latencies.size();
            average_time += avg; 
            total_sum += sum;
        }
        average_time = average_time/iterations;
        total_sum = total_sum/iterations;
        cout << name << " pop:\t" << average_time << " ms. Total:\t" << total_sum << " ms." << endl;
    }

    void testBulkPush(size_t init_size, size_t npush){
        size_t iterations = 10;
        double total_time = 0.0;

        for (size_t iter = 0; iter < iterations; iter++){
            T lfq; // need not be initialized.
            llist new_nodes = get_nodes(npush);

            asm volatile("" ::: "memory");
            auto start = std::chrono::high_resolution_clock::now();
            lfq.b_push(new_nodes);

            auto end = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(end-start).count() * (1e6);
            asm volatile("" ::: "memory");

            total_time += elapsed;
        }
        total_time = total_time /iterations;
        cout << name << " b_push:\t" << total_time << " ms." << endl;
    }


    void testSteal(size_t init_size, double proportion) {
        const size_t iterations = 10;
        double average_time = 0.0;

        for(size_t iter = 0; iter < iterations; iter++){
            T lfq;

            if(init_size){
                llist new_nodes = get_nodes(init_size);
                lfq.b_push(new_nodes);
            }

            double steal_latency = 0.0;

            std::thread consumer ([&]{
                // pop from queue.
                volatile size_t uncached_num = 0;

                node *dummy_node = new node();
                llist new_list = {dummy_node, dummy_node, 1};

                asm volatile("" ::: "memory");
                auto start = std::chrono::high_resolution_clock::now();

                llist popped_node = lfq.b_steal(proportion);

                auto end = std::chrono::high_resolution_clock::now();
                steal_latency = std::chrono::duration<double>(end-start).count();  
                asm volatile("" ::: "memory");

                for (auto start = popped_node.start; start; start = start->next){
                    uncached_num++;
                }
            });
            consumer.join();

            // compute average.
            steal_latency = steal_latency * (1e6);
            average_time += steal_latency; 
        }
        average_time = average_time/iterations;
        cout << name << " steal:\t" << average_time <<" ms." << endl;
    }

    void testOwnerPushPop(size_t init_size, size_t nops){
        double total_time = 0.0;
        vector<double> times(iterations);

        for (size_t iter = 0; iter < iterations; iter++){
            llist init_nodes = get_nodes(init_size);
            T lfq;
            lfq.b_push(init_nodes);

            vector<double> op_latency;

            volatile size_t uncached_num;
            for (size_t i = 0; i < nops; i++){
                if (!(i % 3)){ // push operation.
                    llist new_list = get_nodes(128);

                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();

                    lfq.b_push(new_list);

                    auto end = std::chrono::high_resolution_clock::now();
                    op_latency.push_back(std::chrono::duration<double>(end-start).count() * (1e6));
                    asm volatile("" ::: "memory");
                }
                else { // pop operation
                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();

                    node *popped_node = lfq.pop();

                    auto end = std::chrono::high_resolution_clock::now();
                    op_latency.push_back(std::chrono::duration<double>(end-start).count() * (1e6));
                    asm volatile("" ::: "memory");
                    uncached_num = popped_node->padding[0];
                    delete popped_node;
                }
            }
            double total_time = std::accumulate(op_latency.begin(), op_latency.end(), 0.0);
            times[iter] = total_time; 
        }
        double avg = std::accumulate(times.begin(), times.end(), 0.0)/iterations;
        cout << name << " push-pop:\t" << avg << " ms. " << endl;
    }

    void testPushSteal(size_t init_size, size_t nops){

    }
};

#endif

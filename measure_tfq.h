#include "LFQ.h"

#include "node.h"

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <taskflow/taskflow.hpp>

typedef struct {
    uint8_t padding[56];
    tf_node *next;
} tf_node;

typedef tuple<tf_node *, tf_node *, size_t> l_list;

typedef struct {
    tf_node * start;
    tf_node *end;
    size_t size;
} llist;


class tf_ubq : public tf::UnboundedTaskQueue<tf_node *>{
public:
    using tf::UnboundedTaskQueue<tf_node *>::push;

    void b_push(const l_list& nodes){
        auto [start, end, n] = nodes;
        for (; start ; start = start->next){
            this->push(start);
        }
    }

    l_list b_steal(double proportion){
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

class Measure_tfq{
public:

    void push(size_t init_size, size_t npush){
        double average_time = 0.0;
        double iterations = 10;
        double total_sum = 0.0;

        for (size_t iter = 0; iter < iterations; iter++){
            // tf::UnboundedTaskQueue<tf_node*> tfq;
            tf_ubq tfq;
            vector<double> push_latencies(npush, 0.0);

            std::thread producer([&]{

                for (size_t i = 0; i < npush; i++){
                    tf_node *temp = new tf_node();

                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    tfq.push(temp);

                    auto end = std::chrono::high_resolution_clock::now();
                    push_latencies[i] = std::chrono::duration<double>(end-start).count();
                    asm volatile("" ::: "memory"); // not to reorder the instructions.
                }
            });
            producer.join();

            double sum = 0.0;
            for (double t : push_latencies) sum += (t * 1e6);
            double average = sum/push_latencies.size();
            average_time += average;
            total_sum += sum;
        }
        average_time /= iterations;
        total_sum /= iterations;
        cout << "taskflow - push:\t" << average_time << " ms. Total:\t" << total_sum << " ms." << endl;
    }


    void pop(size_t init_size, size_t npop){
        double average_time = 0.0;
        double iterations = 10;
        double total_sum = 0.0;

        for (size_t iter = 0; iter < iterations; iter++){
            tf::UnboundedTaskQueue<tf_node*> tfq;
            vector<double> pop_latencies(npop, 0.0);

            for (size_t i = 0; i < init_size; i++){
                tf_node *temp = new tf_node();
                tfq.push(temp);
            }

            std::thread producer([&]{

                volatile size_t uncached;
                for (size_t i = 0; i < npop; i++){

                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    auto popped_node = tfq.pop();

                    auto end = std::chrono::high_resolution_clock::now();
                    pop_latencies[i] = std::chrono::duration<double>(end-start).count();
                    asm volatile("" ::: "memory"); // not to reorder the instructions.
                    uncached = popped_node->padding[1];
                }
            });
            producer.join();

            double sum = 0.0;
            for (double t : pop_latencies) sum += (t * 1e6);
            double average = sum/pop_latencies.size();
            average_time += average;
            total_sum += sum;
        }
        average_time /= iterations;
        total_sum /= iterations;
        cout << "taskflow - pop:\t" << average_time << " ms. Total:\t" << total_sum << " ms." << endl;
    }


    void steal(size_t init_size, double proportion){
        double average_time = 0.0;
        double iterations = 10;
        double total_sum = 0.0;

        for (size_t iter = 0; iter < iterations; iter++){
            tf::UnboundedTaskQueue<tf_node*> tfq;

            for (size_t i = 0; i < init_size; i++){
                tf_node *temp = new tf_node();
                tfq.push(temp);
            }

            size_t npop = proportion * init_size;
            vector<double> steal_latencies(npop, 0);

            std::thread producer([&]{

                volatile size_t uncached;
                for (size_t i = 0; i < npop; i++){

                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();
                    
                    auto popped_node = tfq.pop();

                    auto end = std::chrono::high_resolution_clock::now();
                    steal_latencies[i] = std::chrono::duration<double>(end-start).count();
                    asm volatile("" ::: "memory"); // not to reorder the instructions.
                    uncached = popped_node->padding[1];
                }
            });
            producer.join();

            double sum = 0.0;
            for (double t : steal_latencies) sum += (t * 1e6);
            double average = sum/steal_latencies.size();
            average_time += average;
            total_sum += sum;
        }
        average_time /= iterations;
        total_sum /= iterations;
        cout << "taskflow - pop:\t" << total_sum << " ms." << endl;
    }
};


#include "LFQ.h"
#include "lf_queue.h"
#include <vector>
#include <thread>
#include <algorithm>

using namespace std;




class Measure_myq{

public:
    void push(size_t init_size, size_t npush)  {
        // init new queue.
        const size_t iterations = 10;
        double average_time = 0.0;
        double total_sum = 0.0;

        for(size_t iter = 0; iter < iterations; iter++){
            lf_queue lfq; // queue should be empty?
            vector<double> push_latencies(npush, 0.0);

            std::thread producer ([&]{
                // push n iterms to the queue.
                for (size_t i = 0; i < npush; i++){

                    lf_node *dummy_node = new lf_node();
                    llist new_list = {dummy_node, dummy_node, 1};

                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();

                    lfq.push(new_list);

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

        cout << "myq - push:\t" << average_time <<" ms. Total:\t" << total_sum << " ms." << endl;
    }

    void push_bulk(size_t init_size, size_t npush){
        // push all nodes at once.
    }
    void pop(size_t init_size, size_t npop){
        const size_t iterations = 10;
        double average_time = 0.0;
        double total_sum = 0.0;

        for(size_t iter = 0; iter < iterations; iter++){
            lf_queue lfq; // should be initialized.

            for (size_t i = 0; i < init_size; i++){
                lf_node *temp = new lf_node();
                lfq.push({temp, temp, 1});
            }

            vector<double> push_latencies(npop, 0.0);

            std::thread consumer ([&]{
                // pop from queue.
                volatile size_t uncached_num;
                for (size_t i = 0; i < npop; i++){

                    lf_node *dummy_node = new lf_node();
                    llist new_list = {dummy_node, dummy_node, 1};

                    asm volatile("" ::: "memory");
                    auto start = std::chrono::high_resolution_clock::now();

                    lf_node *popped_node = lfq.pop();

                    auto end = std::chrono::high_resolution_clock::now();
                    push_latencies[i] = std::chrono::duration<double>(end-start).count();  
                    asm volatile("" ::: "memory");
                    uncached_num = popped_node->size[0];
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

        cout << "myq - pop:\t" << average_time <<" ms. Total:\t" << total_sum <<" ms." << endl;
    }

    void steal(size_t init_size, double proportion) {
        const size_t iterations = 10;
        double average_time = 0.0;

        for(size_t iter = 0; iter < iterations; iter++){
            lf_queue lfq; // should be initialized.

            for (size_t i = 0; i < init_size; i++){
                lf_node *temp = new lf_node();
                lfq.push({temp, temp, 1});
            }

            double steal_latency = 0.0;

            std::thread consumer ([&]{
                // pop from queue.
                volatile size_t uncached_num = 0;

                lf_node *dummy_node = new lf_node();
                llist new_list = {dummy_node, dummy_node, 1};

                asm volatile("" ::: "memory");
                auto start = std::chrono::high_resolution_clock::now();

                llist popped_node = lfq.m_pop(proportion);

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
        cout << "myq - steal:\t" << average_time <<" ms." << endl;
    }
};
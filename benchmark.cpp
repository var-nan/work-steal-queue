#include "lf_queue.h"
#include "chase_lev_queue.h"

#include <vector>
#include <iostream>
#include <atomic>
#include <chrono>
#include <optional>
#include <cassert>
#include <bitset>
#include <thread>

constexpr size_t TASK_COUNT = 1'000'000;

using namespace std;

class LatencyRecorder{
    std::bitset<5> operations; 
    std::vector<double> pushLat, popLat, stealLat, bulkPushLat, bulkStealLat;

public:
    LatencyRecorder(size_t n): pushLat(n), popLat(n), stealLat(n), bulkPushLat(n), bulkStealLat(n){}

    void report(const std::string& workLoadType){
        // print stats.
    }
};


void bench_my_q_push(){ // workload type: ownwer push some nodes, no stealer.

   lf_queue myq;
   vector<double> pushLat(TASK_COUNT); 
   // init queue.
    for (int i = 0; i < 64; i++){
        lf_node *node = new lf_node();
        myq.push({node, node, 1});
    }
   auto pusher = std::thread([&](){
        for(size_t i = 0; i < TASK_COUNT; i++){
            lf_node *node = new lf_node();
            llist new_node = {node, node, 1};
            asm volatile("" ::: "memory");
            auto t0 = std::chrono::high_resolution_clock::now();
            myq.push(new_node);
            pushLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now() - t0).count();
            asm volatile("" ::: "memory");
        } 
   });

    pusher.join();

    double sum = 0;
    for (double t : pushLat) sum += t;
    double avg = sum/pushLat.size();
    avg = avg * (1e6);
    sum = sum * (1e6);
    cout<< "latency for push operation in lf_queue:\t" << avg << "ms.\t" << sum << " ms. (total)"<< endl;
}

void bench_cl_q_push(){ // workload: owner push some nodes, no stealer.
    deque::Deque<cl_node> clq;
    // init clq with some nodes. 
    vector<double> pushLat(TASK_COUNT);

    for (int i = 0; i< 64; i++){
        clq.push_bottom({342});
    }

    auto pusher = std::thread([&](){

        for(size_t i =0; i < TASK_COUNT; i++){
            cl_node node;
            asm volatile("" ::: "memory");
            auto t0 = std::chrono::high_resolution_clock::now();
            clq.push_bottom(node);
            pushLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now()-t0).count();
            asm volatile("" ::: "memory");
        }
   });

   pusher.join();

    double sum = 0;
    for (double t : pushLat) sum += t;
    double avg = sum/pushLat.size();
    avg = avg * (1e6);
    sum = sum *(1e6);
    cout<< "latency for push operation in cl_queue:\t" << avg << "ms.\t" << sum << " ms. (total)"<< endl;
   // post processing statistic.
}

void bench_my_q_pop(){ // workload: owner pop nodes, no stealer.
    lf_queue myq;
   vector<double> popLat(TASK_COUNT); 
   // init queue.
    for (size_t i = 0; i < (TASK_COUNT+64); i++){
        lf_node *node = new lf_node();
        myq.push({node, node, 1});
    }
   auto pusher = std::thread([&](){
        volatile size_t volatile_number;
        for(size_t i = 0; i < TASK_COUNT; i++){
            asm volatile("" ::: "memory");
            auto t0 = std::chrono::high_resolution_clock::now();
            lf_node *popped_node= myq.pop();
            popLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now() - t0).count();
            asm volatile("" ::: "memory");
            volatile_number = popped_node->size[0];
        } 
   });

    pusher.join();

    double sum = 0;
    for (double t : popLat) sum += t;
    double avg = sum/popLat.size();
    avg = avg * (1e6);
    sum = sum * (1e6);
    cout<< "latency for pop operation in lf_queue:\t" << avg << "ms.\t" << sum << " ms. (total)"<< endl;
}

void bench_cl_q_pop(){
    deque::Deque<cl_node> clq;
    // init clq with some nodes. 
    vector<double> pushLat(TASK_COUNT);

    for (int i = 0; i< (64+TASK_COUNT); i++){
        clq.push_bottom({342});
    }

    auto pusher = std::thread([&](){

        volatile size_t volatile_number;

        for(size_t i =0; i < TASK_COUNT; i++){
            asm volatile("" ::: "memory");
            auto t0 = std::chrono::high_resolution_clock::now();
            auto popped_node = clq.pop_bottom();
            pushLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now()-t0).count();
            volatile_number = popped_node.value().n[0];
            asm volatile("" ::: "memory");
        }
   });

   pusher.join();

    double sum = 0;
    for (double t : pushLat) sum += t;
    double avg = sum/pushLat.size();
    avg = avg * (1e6);
    sum = sum * (1e6);
    cout<< "latency for pop operation in cl_queue:\t" << avg << "ms.\t" << sum << " ms. (total)"<< endl;
}

void bench_lf_q_push_pop(size_t total_op){ // owner, push and pop operations, no stealer.

    // init the queue with 100,000 nodes.
    // remove 10K from either top or bottom.
    /*
        init the queue with 100K nodes.
        remove 10K from either top or bottom.
        mixture of 5k push and 5k pop operations.
        measure average execution time of each operation.
    */
   lf_queue myq;
   vector<double> opLat(total_op); 
   // init queue.
    for (size_t i = 0; i < (TASK_COUNT); i++){
        lf_node *node = new lf_node();
        myq.push({node, node, 1});
    }
    // remove 10K nodes from top.
    volatile lf_node *removed_ptr;
    for (size_t i =0; i < total_op; i++){
        removed_ptr = myq.pop();
    }

   auto pusher = std::thread([&](){

        volatile lf_node *popped_node;
        for(size_t i = 0; i < total_op; i++){
            if (i&1) { // push op.
                lf_node *node = new lf_node();
                llist new_node = {node, node, 1};

                asm volatile("" ::: "memory");
                auto t0 = std::chrono::high_resolution_clock::now();
                myq.push(new_node);
                opLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now() - t0).count();
                asm volatile("" ::: "memory");
            }
            else { // pop op.
                asm volatile("" ::: "memory");
                auto t0 = std::chrono::high_resolution_clock::now();
                popped_node= myq.pop();
                opLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now() - t0).count();
                asm volatile("" ::: "memory");
            }
        } 
   });

    pusher.join();

    double sum = 0;
    for (double t : opLat) sum += t;
    double avg = sum/opLat.size();
    avg = avg * (1e6);
    sum = sum * (1e6);
    cout<< "latency for push/pop operation in lf_queue:\t" << avg << "ms.\t" << sum << " ms. (total)"<< endl;
}


void bench_cl_q_push_pop(size_t total_op){

    deque::Deque<cl_node> clq;
    vector<double> opLat(total_op);

    // init queue.
    for (size_t i = 0; i < (TASK_COUNT); i++){
        clq.push_bottom({345});
    }
    // remove 10K nodes from top.
    volatile size_t removed_ptr;
    for (size_t i =0; i < total_op; i++){
        removed_ptr = clq.pop_bottom().value().n[0];
    }

   auto pusher = std::thread([&](){

        volatile size_t popped_node;
        for(size_t i = 0; i < total_op; i++){
            if (i&1) { // push op.

                asm volatile("" ::: "memory");
                auto t0 = std::chrono::high_resolution_clock::now();
                clq.push_bottom({i});
                opLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now() - t0).count();
                asm volatile("" ::: "memory");
            }
            else { // pop op.
                asm volatile("" ::: "memory");
                auto t0 = std::chrono::high_resolution_clock::now();
                popped_node= clq.pop_bottom().value().n[0];
                opLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now() - t0).count();
                asm volatile("" ::: "memory");
            }
        } 
   });

    pusher.join();

    double sum = 0;
    for (double t : opLat) sum += t;
    double avg = sum/opLat.size();
    avg = avg * (1e6);
    sum = sum * (1e6);
    cout<< "latency for push/pop operation in cl_queue:\t" << avg << "ms.\t" << sum << " ms. (total)"<< endl;

}

void lf_q_m_pop_bulk(double proportion){

    lf_queue myq;
    vector<double> bulkPopLat;

    for (size_t i = 0; i < (TASK_COUNT); i++){
        lf_node *node = new lf_node();
        myq.push({node, node, 1});
    }

    volatile lf_node *start_node;
    // perform bunch of removals from bottom
    asm volatile("" ::: "memory"); 
    auto t0 = std::chrono::high_resolution_clock::now();
    llist popped_result= myq.m_pop(proportion);
    start_node = popped_result.start;
    auto res = std::chrono::high_resolution_clock::now();
    asm volatile("":::"memory");
    double duration = std::chrono::duration<double>(res-t0).count();
    duration = duration * 1e6;
    // verify that n is equal.
    size_t count = 0;
    for (; start_node; start_node= start_node->next) count++;
    if(count != popped_result.n) cout << popped_result.n << " " << count << " different" << endl;
    cout << "latency of bulk pop operation " << popped_result.n << " nodes:\t" << duration << endl;
}


void bench_cl_q_m_pop_bulk(double proportion){
    deque::Deque<cl_node> clq;
    size_t n_removed = static_cast<size_t>((double)TASK_COUNT* proportion);

    // init clq with some nodes. 
    vector<double> pushLat(n_removed);

    for (int i = 0; i< (TASK_COUNT); i++){
        clq.push_bottom({342});
    }


    auto pusher = std::thread([&](){

        volatile size_t volatile_number;

        for(size_t i =0; i < n_removed; i++){
            asm volatile("" ::: "memory");
            auto t0 = std::chrono::high_resolution_clock::now();
            auto popped_node = clq.steal();
            pushLat[i] = std::chrono::duration<double>(chrono::high_resolution_clock::now()-t0).count();
            volatile_number = popped_node.value().n[0];
            asm volatile("" ::: "memory");
        }
   });

   pusher.join();

    double sum = 0;
    for (double t : pushLat) sum += t;
    // double avg = sum/pushLat.size();
    // avg = avg * (1e6);
    sum = sum * (1e6);
    cout<< "latency for steal operation ("<<n_removed << ") nodes in cl_queue:\t" << sum << " ms. (total)"<< endl;
}


int main(){
    cout << "chase lev queue" << endl;
    // bench_cl_q_push();
    // bench_cl_q_pop();
    //bench_cl_q_push_pop(100000);
    bench_cl_q_m_pop_bulk(0.1);
    cout << "nandhan queue" << endl;
    // bench_my_q_push();
    // bench_my_q_pop();

    //bench_lf_q_push_pop(100000);

    lf_q_m_pop_bulk(0.1);

    return 0;
}


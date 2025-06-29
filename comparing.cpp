#include <iostream>
#include "node.h"
#include "utils.h"
#include "lf_queue.h"
#include <vector>
#include <thread>

using namespace std;


template<typename T>
class Explorer {
private:
    std::vector<Worker<T>> payloads;
    std::vector<thread> threads;
    size_t nworkers;
    size_t levels;
    size_t branch_factor;
    
public:
    Explorer(size_t nworkers_, size_t levels_, size_t bf): nworkers{nworkers_}, levels{levels_}, branch_factor{bf}{

    }

    void supervise(){

        vector<uint32_t> worker_status(nworkers, 0);

        MasterQ private_q;

        for(;;){
            size_t idle = 0, processing =0;

            // worker_status.clear();

            for (size_t i = 0; i < nworkers; i++){
                auto& worker = payloads[i];
                auto st = worker.status.load(std::memory_order::acquire);
                
                if (st == 1){ // worker needs nodes.
                    size_t q_size = private_q.get_size();
                    if (q_size){
                        size_t k = (q_size & 1) ? (1 + q_size>>1) : (q_size>>1);
                        llist new_list = private_q.pop_k(k);
                        worker.local_q.b_push(new_list);

                        worker.status.store(0, std::memory_order::release);
                        worker.status.notify_one();
                        worker_status[i] = 0; 
                        processing++;
                        // cout <<"sent nodes" << endl;
                    }
                    else {
                        idle++;
                        worker_status[i] = 1;
                    }
                }
                else {
                    processing++;
                    worker_status[i] = 0;
                }
            }

            // cout << "idle: " << idle << "working: " << processing << endl;

            if(idle == nworkers && private_q.empty()) {
                for (size_t i =0 ; i < nworkers; i++){
                    auto& worker = payloads[i];
                    worker.status.store(2, std::memory_order::release);
                    worker.status.notify_one();
                }
                break;
            }

            for (size_t i = 0; i < nworkers && idle ; i++){
                if (worker_status[i]) continue;

                auto& worker = payloads[i];
                auto st = worker.status.load(std::memory_order::acquire);
                if (st) continue;
                llist new_list = worker.local_q.b_steal(0.5);
                if (!new_list.start) continue; // either empty or not enough nodes.
                private_q.push(new_list);
            }
        }
        
        cout << "Done" << endl;
    }

    void start(size_t levels_, size_t bf){
        levels = levels_;
        branch_factor = bf;

        payloads = vector<Worker<T>>(nworkers);

        // start n threads

        for (size_t i = 0; i < nworkers; i++){
            // payloads.push_back(Worker<T>());
            threads.push_back(std::thread(&Explorer::explore, this, i));
        }

        supervise();
        // wait for threads
        for (size_t i = 0; i < nworkers; i++){
            threads[i].join();
        }

        for (const auto& payload: payloads)
            cout << payload.tasks_processed << " "; cout << endl;
        cout<< "Completed" << endl;
    }


    void explore(size_t id){

        std::mt19937 gen(12345+id);
    std::uniform_int_distribution<uint32_t> dist(10, 1000);

        auto& payload = payloads[id];
        auto& local_q = payload.local_q;

        // push a node to queue.
        Node *startNode = new Node();
        startNode->id = 0;
        startNode->layerNo = 0;
        startNode->sleeptime = dist(gen);
        local_q.b_push({startNode, startNode, 1});

        uint32_t status = 0;

        for (; status != 2;){
            
            for (Node *ptr; (ptr = local_q.pop()); ){

                Node current = *ptr;

                if(current.layerNo >= levels) continue;
                // continue;

                // pretend working and push new nodes to queue.
                this_thread::sleep_for(std::chrono::microseconds(current.sleeptime));

                payload.tasks_processed++;

                if (dist(gen) < 100) continue;
                
                // create linked list
                // if (current.layerNo >= levels) continue;
                llist new_nodes ; // generate new nodes
                {
                    Node *start = new Node();
                    Node *curr = start;
                    for (size_t bf = 0; bf < branch_factor; bf++){
                        Node *temp = new Node();
                        temp->layerNo = (current.layerNo)+1;
                        temp->sleeptime = dist(gen);
                        curr->next = temp;
                        curr = curr->next;
                    }
                    new_nodes.start = start->next;
                    new_nodes.end = curr;
                    new_nodes.size = branch_factor;
                }
                // cout << id <<endl;
                local_q.b_push(new_nodes);
            }

            // ask master and sleep.
            // cout << "waiting for master to send nodes" << endl;
            payload.status.store(1, std::memory_order::release);
            payload.status.wait(1); // sleep until master signals.
            status = payload.status.load(std::memory_order::acquire);
        }
    }
    
};


int main(){
    cout << "starting" << endl;
    Explorer<lf_queue> explorer(4, 6,5);
    explorer.start(8,5);
    cout << "end" << endl;
}
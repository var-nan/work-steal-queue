#include <iostream>
#include "node.h"
#include "utils.h"
#include "lf_queue.h"
#include "tf_queue.h"
#include "chase_lev_q.h"
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
    std::vector<std::atomic<bool>> visited;
    std::vector<double> bursts;
    
public:
    Explorer(size_t nworkers_, size_t levels_, size_t bf): 
            nworkers{nworkers_}, levels{levels_}, branch_factor{bf}, visited(100000){
    }

    void supervise(){

        vector<uint32_t> worker_status(nworkers, 0);

        MasterQ private_q;

        double total_steal_time = 0.0, total_push_time = 0.0;
        size_t total_steals = 0.0, total_pushes = 0.0;

        for(;;){
            size_t idle = 0, processing =0;

            // worker_status.clear();

            for (size_t i = 0; i < nworkers; i++){
                auto& worker = payloads[i];
                auto st = worker.status.load();
                
                if (st == 1){ // worker needs nodes.
                    size_t q_size = private_q.get_size();
                    if (q_size){
                        // size_t k = (q_size & 1) ? (1 + q_size>>1) : (q_size>>1);
                        size_t k = static_cast<size_t>(ceil(q_size * 0.4));
                        llist new_list = private_q.pop_k(k);

                        auto start = std::chrono::high_resolution_clock::now();
                        worker.local_q.b_push(new_list);
                        auto end = std::chrono::high_resolution_clock::now();
                        total_push_time += std::chrono::duration<double>(end -start).count();
                        total_pushes += new_list.size;                        

                        worker.status.store(0);
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
                    worker.status.store(2);
                    worker.status.notify_one();
                }
                // cout << "indicated finish" << endl;
                break;
            }

            for (size_t i = 0; i < nworkers && idle; i++){
                if (worker_status[i]) continue;

                auto& worker = payloads[i];
                auto st = worker.status.load(std::memory_order::acquire);
                if (st) continue;
                // instrument the code.
                auto start = std::chrono::high_resolution_clock::now();
                llist new_list = worker.local_q.b_steal(0.5);
                auto end = std::chrono::high_resolution_clock::now();
                
                if (!new_list.start) continue; // either empty or not enough nodes.

                total_steal_time += std::chrono::duration<double>(end-start).count();
                total_steals += new_list.size; 
                private_q.push(new_list);
            }
        }


        cout << "Throughput - steal: " << total_steals/total_steal_time << " , push: " << total_pushes/total_push_time << endl;
        
        // cout << "Done" << endl;
    }

    void start(size_t levels_, size_t bf){
        levels = levels_;
        branch_factor = bf;

        payloads = vector<Worker<T>>(nworkers);
        
        // start n threads
        for (size_t i = 0; i < nworkers; i++){
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

    void start2(IGraph& g){
        payloads = vector<Worker<T>>(nworkers);
        
        bursts = std::vector<double>(nworkers);

        // find roots for the graph.
        std::vector<size_t> indegree(g.size(), 0);
        for (size_t i = 0; i < g.size(); i++){
            for (const auto& nb : g[i].childs){
                indegree[nb]++;
            }
        }

        std::vector<size_t> roots;
        for (size_t i = 0; i < indegree.size(); i++){
            if (indegree[i])continue;
            roots.push_back(i);
        }

        // typically number of roots should be equal to number of threads.
        for (size_t i =0; i < roots.size(); i++){
            size_t root_id = roots[i];
            auto& payload = payloads[i%nworkers];
            llist temp = {&g[root_id], &g[root_id], 1};
            payload.local_q.b_push(temp);
            // threads.push_back(std::thread(&Explorer::dfs, this, std::ref(g), i));
        }

        // check number of nodes in each paylaod

        // cout << "payload sizes: ";
        // for (size_t i = 0; i < nworkers; i++){
        //     const auto& payload = payloads[i];
        //     cout << payload.local_q.size() << " ";
        // } cout << endl;


        for (size_t i = 0; i < nworkers; i++){
            threads.push_back(std::thread(&Explorer::dfs, this, std::ref(g), i));
        }

        supervise();
        for (size_t i = 0; i < nworkers; i++){
            threads[i].join();
        }

        // for (const auto& payload: payloads) cout << payload.tasks_processed << " "; cout << endl;
        // cout << "Burst times: "; for (auto burst : bursts) cout << burst << " "; cout << endl;
        cout << "Completed" << endl;
        cout << endl;
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
    
    void dfs(IGraph& graph, size_t id){
        auto& payload = payloads[id];
        auto& local_q = payload.local_q;

        uint32_t status = payload.status.load();
        // cout << "Starting dfs" << endl;

        auto burst_start = std::chrono::high_resolution_clock::now();

        for (; status != 2;){

            for (Node *ptr; (ptr = local_q.pop()); ){
                Node current = *ptr;
                uint32_t u = current.id;
                // pseudo work // sleep for some time.
                std::this_thread::sleep_for(std::chrono::microseconds(current.sleeptime)); 

                // discover neighbors.
                // if (visited[u].exchange(true)) continue; // don't think this is needed here.

                payload.tasks_processed++;

                Node *start = new Node();
                Node *curr = start;
                size_t count = 0;
                for (const auto& v : graph[u].childs){
                    // auto& child = graph[v];
                    if(!visited[v].exchange(1)){
                        curr->next = &graph[v];
                        curr = curr->next;
                        count++;
                    }
                }
                if (count){
                    llist children = {start->next, curr, count};
                    local_q.b_push(children);
                }
                delete start;
            }
            // queue is empty, indicate master.
            // cout << "Worker needs nodes" << endl;
            payload.status.store(1);
            payload.status.wait(1);
            status = payload.status.load();
        }
        auto burst_end = std::chrono::high_resolution_clock::now();
        bursts[id] = std::chrono::duration<double>(burst_end-burst_start).count();
    }
};


int main(int argc, char *argv[]){
    size_t nWorkers = 2;
    if (argc == 2) nWorkers = (size_t)atol(argv[1]);

    IGraph graph;
    parallel_graph(graph, 10000, 10000000, nWorkers); 

    cout << "Task flow work stealing queue" << endl;
    Explorer<tf_ub_queue> explorer(nWorkers, 6,5);
    explorer.start2(graph);

    cout << "Nandhan's queue." << endl;

    Explorer<lf_queue> explorer2(nWorkers, 6,5);
    explorer2.start2(graph);

    cout << "Task flow bounded queue" << endl;
    Explorer<tf_bq> explorer3(nWorkers, 6,5);
    explorer3.start2(graph);
    cout << "end" << endl;
}
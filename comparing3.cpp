#include <iostream>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <tuple>
#include <random>
#include <chrono>
#include <cassert>

#include "node.h"
#include "lf_queue.h"
#include "tf_queue.h"

using namespace std;

typedef node Node;

// ==== Graph Node Definition ====
//class Node {
//public:
//    atomic<bool> visited = false;
//    vector<Node*> outgoing;
//    Node* next = nullptr; // For linked list chaining
//};


// ==== Graph Structure ====
class Graph {
public:
    vector<Node*> roots;
    // unordered_map<Node*, Node> nodes; // not needed.
};

// ==== Type for Linked List ====
// typedef tuple<Node*, Node*, size_t> l_list;

// ==== Worker Template ====
template <typename T>
class Worker {
public:
    Worker() = default;
    // move ctor and assign operator.
    // Worker(Worker&&) = default;
    // Worker& operator=(Worker&&) = default;
    
    T local_q;
    atomic_flag stealing = ATOMIC_FLAG_INIT;
    size_t tasks_processed = 0;
    size_t steals_performed = 0;
    size_t failed_steal_attempts = 0;
    chrono::duration<double> active_time = chrono::duration<double>::zero();
};

// ==== DFSExplore Driver ====
template <typename T>
class DFSExplore {
private:
    vector<Worker<T>> payloads;
    size_t nworkers;

public:
    DFSExplore(size_t nworkers_) : nworkers(nworkers_) , payloads{nworkers_}{
        for(size_t i = 0; i < nworkers; i++){
            // payloads.push_back(Worker<T>());
            // payloads.emplace_back();
        }
        // payloads.resize(nworkers_);
    }

    void explore(const Graph& graph) {
        assert(graph.roots.size() == nworkers && "Number of roots must match number of workers");

        // Initialize each worker queue with a root node
        for (size_t i = 0; i < nworkers; ++i) {
            Node* root = graph.roots[i];
            llist initial = {root, root, 1};
            (payloads[i].local_q).b_push(initial);
        }

        vector<thread> threads(nworkers);

        auto start = chrono::high_resolution_clock::now();

        auto dfs_fn = [this](size_t tid) {
            auto& worker = payloads[tid];
            auto local_start = chrono::high_resolution_clock::now();

            while (true) {
                Node* current = (worker.local_q).pop();
                if (!current) {
                    // Attempt to steal from others
                    bool stolen = false;
                    for (size_t j = 0; j < nworkers; ++j) {
                        if (j == tid) continue;
                        auto& victim = payloads[j];
                        if (!victim.stealing.test_and_set()) {
                            llist stolen_nodes = (victim.local_q).b_steal(0.5);
                            victim.stealing.clear();
                            if (stolen_nodes.size > 0) {
                                (worker.local_q).b_push(stolen_nodes);
                                ++worker.steals_performed;
                                stolen = true;
                                break;
                            } else {
                                ++worker.failed_steal_attempts;
                            }
                        } else {
                            ++worker.failed_steal_attempts;
                        }
                    }
                    if (!stolen) break; // No more work
                    continue;
                }

                if (!current->visited.exchange(true)) {
                    ++worker.tasks_processed;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // processing some work.
                    // Push unvisited children to local queue
                    Node* head = nullptr;
                    Node* tail = nullptr;
                    size_t count = 0;
                    for (Node* child : current->outgoing) {
                        if (!child->visited.load()) {
                            if (!head) head = tail = child;
                            else {
                                tail->next = child;
                                tail = child;
                            }
                            ++count;
                        }
                    }
                    if (count > 0) {
                        tail->next = nullptr;
                        llist child_list = {head, tail, count};
                        (worker.local_q).push(child_list);
                    }
                }
            }

            auto local_end = chrono::high_resolution_clock::now();
            worker.active_time = local_end - local_start;
        };

        for (size_t i = 0; i < nworkers; ++i)
            threads[i] = thread(dfs_fn, i);
        for (auto& t : threads)
            t.join();

        auto end = chrono::high_resolution_clock::now();
        double duration = chrono::duration<double>(end - start).count();

        // Gather metrics
        size_t total_tasks = 0, total_steals = 0, total_failed_steals = 0;
        chrono::duration<double> total_active_time = chrono::duration<double>::zero();

        for (size_t i = 0; i < nworkers; ++i) {
            total_tasks += payloads[i].tasks_processed;
            total_steals += payloads[i].steals_performed;
            total_failed_steals += payloads[i].failed_steal_attempts;
            total_active_time += payloads[i].active_time;

            cout << "Worker " << i
                 << ": Tasks=" << payloads[i].tasks_processed
                 << ", Steals=" << payloads[i].steals_performed
                 << ", Failed Steals=" << payloads[i].failed_steal_attempts
                 << ", Time=" << payloads[i].active_time.count() << "s\n";
        }

        double throughput = static_cast<double>(total_tasks) / duration;

        cout << "\n===== Summary =====\n";
        cout << "Total tasks processed: " << total_tasks << "\n";
        cout << "Total steals performed: " << total_steals << "\n";
        cout << "Total failed steals: " << total_failed_steals << "\n";
        cout << "Total execution time: " << duration << " seconds\n";
        cout << "Average worker active time: " << (total_active_time.count() / nworkers) << " seconds\n";
        cout << "Throughput: " << throughput << " tasks/sec\n";
        cout << "Scaling Efficiency (active_time / wall_time): " << (total_active_time.count() / (duration * nworkers)) * 100 << "%\n";
    }
};

// ==== DAG Graph Generator ====
Graph generate_dag(size_t n_roots) {
    Graph g;
    mt19937 rng(random_device{}());
    uniform_int_distribution<int> depth_dist(4, 6);
    uniform_int_distribution<int> outdegree_dist(2, 7);

    int depth = depth_dist(rng);
    vector<vector<Node*>> levels(depth);

    // Generate root nodes
    for (size_t i = 0; i < n_roots; ++i) {
        Node *nd = new Node();
        // g.nodes[nd] = nd;
        g.roots.push_back(nd);
        levels[0].push_back(nd);
        // levels[0].push_back(&g.nodes[&node]);
    }

    // Build layered DAG
    for (int d = 1; d < depth; ++d) {
        int prev_level_size = levels[d - 1].size();
        int curr_level_size = prev_level_size * 2;  // Ensure density
        for (int i = 0; i < curr_level_size; ++i) {
            Node* new_node = new Node();
            levels[d].push_back(new_node);

            // Randomly connect to previous level
            for (int j = 0; j < outdegree_dist(rng); ++j) {
                Node* parent = levels[d - 1][rng() % prev_level_size];
                parent->outgoing.push_back(new_node);
            }
        }
    }

    return g;
}

int main(){
	size_t n = 2;
	Graph g = generate_dag(n);
    std::cout << "Graph generated" << std::endl;
	DFSExplore<lf_queue> explore(n);
	explore.explore(g);

    std::cout << "LFQ Complted" << std::endl;
    DFSExplore<tf_ub_queue> explore2(n);
    explore2.explore(g);

    std::cout << "Program completed" << std::endl;
}

#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <random>
#include <iostream>
#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include "node.h"


using namespace std;

// typedef struct Node{
//     uint32_t id;
//     uint32_t layerNo;
//     uint32_t sleeptime;
//     std::vector<pair<uint32_t, uint32_t>> children;
//     Node *next;
// } Node;

typedef node Node;

typedef std::vector<std::vector<Node>> Graph;

typedef std::unordered_map<size_t, Node> IGraph;

void parallel_graph(IGraph& nodes, size_t num_nodes, size_t num_edges, size_t roots){

    std::mt19937 rng(47);
    std::uniform_int_distribution<uint32_t> sleeptime(500, 1000);

    std::vector<size_t> order(num_nodes);
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng);

    std::unordered_set<size_t> root_set(order.begin(), order.begin()+roots);
    std::unordered_set<size_t> existing_edges;
    size_t added_edges = 0;

    for (uint32_t i = 0; i < num_nodes; i++){
        Node nd;
        nd.id = i;
        nd.sleeptime = sleeptime(rng);
        // nodes[i] = nd;
        nodes.insert(make_pair(i, nd)); // TODO: fix this.
    }

    while(added_edges < num_edges){
        size_t src = rng()%num_nodes;
        size_t tgt = rng()%num_nodes;

        if (src > tgt) continue;

        size_t u = order[src], v = order[tgt];
        if (root_set.count(v)) continue; // no incoming edge to root.

        uint64_t code = (uint64_t)u * num_nodes + v;
        if (existing_edges.count(code)) continue;

        nodes[u].childs.push_back(v);
        existing_edges.insert(code);
        added_edges++;
    } // the graph is a DAG, with multiple roots.
}

Graph getGraph(size_t layers, size_t roots, size_t branch_factor){

    Graph graph;

    std::mt19937 rng(47);
    std::uniform_int_distribution<uint32_t> sleeptime(10,1000);
    
    vector<Node> currentLayer(roots);
    // build current layer.
    for (size_t i = 0; i < roots; i++){
        Node childNode;
        childNode.id = i;
        childNode.sleeptime = sleeptime(rng);
        childNode.layerNo = 0; 
    }

    graph.push_back(currentLayer);

    // size_t curr_id = roots;

    for (size_t i = 0; i < layers; i++){
        vector<Node> nextLayer;
        uint32_t node_id =0;
        nextLayer.reserve(graph[i].size() * branch_factor);
        for (auto& curr : graph[i]){
            // extract new nodes.
            for(size_t j = 0; i < branch_factor; j++, node_id++){
                Node childNode;
                childNode.id = node_id;
                childNode.layerNo =  i+ 1;
                childNode.sleeptime = sleeptime(rng);
                curr.children.push_back({j, node_id});
                nextLayer.push_back(childNode);
            }
        }
        graph.push_back(std::move(nextLayer));
    }

    return graph;
}


template <typename T>
class Worker{
public:
    T local_q;
    std::atomic_flag stealing; // init with false.
    std::atomic<uint32_t> status = 0;
    size_t tasks_processed = 0;
    size_t tasks_stealed = 0;
};


class MasterQ{
    vector<Node *> nodes;

public:

    MasterQ(){
        nodes.reserve(100000);
    }

    llist pop_k(size_t k) {
        Node *head = new Node ();
        Node *current = head;
        for (size_t i = 0; i < k; i++){
            current->next = nodes.back();
            current = current->next;
            nodes.pop_back(); 
        }
        return {head->next, current, k};
    }

    void push(llist new_nodes) {
        for (Node *start = new_nodes.start; (start) ; start = start->next)
            nodes.push_back(start);
    }

    size_t get_size() const { return nodes.size(); }

    bool empty() const { return nodes.empty(); }
};


#endif // UTILS_H
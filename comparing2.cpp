#include <iostream> 
#include "utils.h"

template<typename T>
class Explorer2{
private:
    std::vector<Worker<T>> payloads;
    size_t nworkers;
public:
    Explorer(size_t nworkers): nworkers{nworkers}{

    }

    void run(size_t id){
        Worker<T>& payload = payloads[id];
        
    }
};

template <typename T>
class DFSExplore {
private:
	std::vector<Worker<T>> payloads;
	size_t nworkers;
	
	DFSExplore(size_t nworkers_): nworkers{nworkers_} {}
	
	void explore(const Graph& graph ){
        	
	}
};

int main(){

}

#include <iostream>
#include "build/_deps/taskflow-src/taskflow/taskflow.hpp"
#include "node.h"

using namespace std;

int main(){
    tf::BoundedTaskQueue<node *, 10> tfbq;
    for (int i = 1; i < (1<<18); i++){
        node *tmp = new node();
        if(!(tfbq.try_push(tmp))) {
            cout << "push failed:" << endl;
        }

    }
    cout << "Size: " << tfbq.size() << endl;
    return 0;
}
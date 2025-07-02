#include <iostream>
// #include "measure_myq.h"
// #include "measure_tfq.h"

#include "LFQ.h"
#include "lf_queue.h"
#include "tf_queue.h"
#include "chase_lev_queue.h"

using namespace myqueue;

int main(){
    std::cout << "pavani nandhan" << std::endl;

    // Measure_myq myq;
    // Measure_tfq tfq;

    // // myq.push(1000,100000);
    // // tfq.push(1000,100000);

    // // myq.pop(1000000, 10000);
    // // tfq.pop(1000000, 10000);
    // myq.steal(100000, 0.05);
    // tfq.steal(100000, 0.05);

    // myq.steal(100000, 0.25);
    // tfq.steal(100000, 0.25);

    // myq.steal(100000, 0.5);
    // tfq.steal(100000, 0.5);

    // myq.steal(100000, 0.75);
    // tfq.steal(100000, 0.75);

    // myq.steal(100000, 0.90);
    // tfq.steal(100000, 0.90);

    TestQ<lf_queue> lfq("Nandu Q");
    lfq.testPush(1000, 100000);
    lfq.testPop(100000, 10000);
    lfq.testBulkPush(1000, 100);
    lfq.testSteal(100000, 0.5);
    lfq.testOwnerPushPop(100, 10000);

    TestQ<tf_ub_queue> tfubq("Tf ub Q");
    tfubq.testPush(1000, 100000); 
    tfubq.testPop(100000, 10000);
    tfubq.testBulkPush(1000, 100);
    tfubq.testSteal(100000, 0.50);
    tfubq.testOwnerPushPop(100, 10000);

    TestQ<cl_queue> clq("Chase Q");
    clq.testPush(1000, 100000);
    clq.testPop(100000, 10000);
    clq.testBulkPush(1000, 100);
    clq.testSteal(100000, 0.5);
    clq.testOwnerPushPop(100, 10000);

    return 0;
}
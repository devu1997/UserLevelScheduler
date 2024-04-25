#pragma once

#include <queue>
#include <deque>
#include "task.h"


// #define MIN_PRIORITY          PRI_MIN_INTERACT
// #define NPQUEUE               PRI_INTERACT_RANGE
#define NPQUEUE               100
#define MIN_PRIORITY          0

class PriorityQueue {
private:
    std::vector<std::deque<Task*>> queue;
    size_t size_;
    std::vector<bool> bitmap;

public:
    PriorityQueue();

    void addTask(Task* task, int priority);
    Task* getNextTask();
    bool empty();
    size_t size();
};
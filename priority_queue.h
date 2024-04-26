#pragma once

#include <queue>
#include <deque>
#include "task.h"


// #define PRIORITYQ_MIN_PRIORITY PRI_MIN_INTERACT
// #define PRIORITYQ_MAX_PRIORITY PRI_MAX_INTERACT
// #define PRIORITYQ_NQUEUE       PRI_INTERACT_RANGE
#define PRIORITYQ_MIN_PRIORITY (SCHED_INTERACT_MIN + MIN_NICENESS)
#define PRIORITYQ_MAX_PRIORITY (SCHED_INTERACT_THRESH - 1)
#define PRIORITYQ_NQUEUE       (PRIORITYQ_MAX_PRIORITY + 1 - PRIORITYQ_MIN_PRIORITY)

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
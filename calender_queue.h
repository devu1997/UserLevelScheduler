#pragma once

#include <vector>
#include <deque>
#include <chrono>
#include "priorities.h"
#include "task.h"


#define NQUEUE               (PRI_MAX_BATCH + 1 - PRI_MIN_BATCH)
#define INSQ_UPDATE_INTERVAL 10

class CalenderQueue {
private:
    std::vector<std::deque<Task*>> queue;
    size_t size_;
    int runq;
    int insq;
    std::chrono::steady_clock::time_point last_insq_update_time;

public:
    CalenderQueue();

    void addTask(Task* task, int priority);
    Task* getNextTask();
    bool empty();
    size_t size();
};
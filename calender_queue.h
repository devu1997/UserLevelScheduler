#pragma once

#include <vector>
#include <deque>
#include <chrono>
#include "priorities.h"
#include "task.h"


// #define CALENDER_MIN_PRIORITY  PRI_MIN_BATCH
// #define CALENDERQ_MAX_PRIORITY PRI_MAX_BATCH 
// #define CALENDERQ_NQUEUE       PRI_BATCH_RANGE
#define CALENDERQ_MIN_PRIORITY SCHED_INTERACT_THRESH 
#define CALENDERQ_MAX_PRIORITY SCHED_INTERACT_MAX + MAX_NICENESS 
#define CALENDERQ_NQUEUE       (CALENDERQ_MAX_PRIORITY + 1 - CALENDERQ_MIN_PRIORITY)
#define INSQ_UPDATE_INTERVAL   10

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
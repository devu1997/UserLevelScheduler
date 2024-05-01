#pragma once

#include <deque>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "logger.h"
#include "task.h"
#include "scheduler.h"
#include "filescheduler.h"


#define MAX_TASKS_TO_STEAL 10

class Coordinator {
private:
    FileScheduler* file_scheduler;
    std::vector<Scheduler*> schedulers;
    int next_scheduler_id = 0;
    
public:
    Coordinator();
    ~Coordinator();

    void submit(Task* task);
    int stealTasks(Scheduler* scheduler);
    void balanceLoad();
    void start();
    void stop();
};

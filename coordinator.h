#pragma once

#include <deque>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "task.h"
#include "scheduler.h"
#include "filescheduler.h"


class Coordinator {
private:
    FileScheduler* file_scheduler;
    std::vector<Scheduler*> schedulers;
    int next_scheduler_id = 0;
    
public:
    Coordinator();
    ~Coordinator();

    void submit(Task* task);
    void start();
    void stop();
};

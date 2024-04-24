#pragma once

#include <deque>
#include <unordered_map>
#include <atomic>
#include "task.h"
#include "priority_queue.h"
#include "filescheduler.h"


#define MIN_NICENESS -20
#define MAX_NICENESS 20

class Scheduler {
private:
    int id;
    FileScheduler *file_scheduler;
    PriorityQueue batch_task_queue;
    std::atomic<bool> stop_flag;
    
public:
    Scheduler(int id);
    ~Scheduler();

    void submit(Task* task);
    void start();
    void stop();
    void setFileScheduler(FileScheduler *file_scheduler);

private:
    void process_interactive_tasks();
};

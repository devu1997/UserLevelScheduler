#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include "task.h"

class FileScheduler;

class Scheduler {
private:
    std::queue<Task*> cpu_task_queue;
    std::mutex cpu_mtx;
    std::atomic<bool> stop_flag;
    
public:
    Scheduler();
    ~Scheduler();

    void submit(Task* task);
    void process_interactive_tasks();
    // void start();
    void stop();
};

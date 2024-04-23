#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include "task.h"
#include "fileioscheduler.h"


class Scheduler {
private:
    FileScheduler *file_scheduler;
    std::queue<Task*> cpu_task_queue;
    std::mutex cpu_mtx;
    std::atomic<bool> stop_flag;
    std::mutex io_mtx;
    
public:
    Scheduler();
    ~Scheduler();

    void submit(Task* task);
    void start();
    void stop();
    void setFileScheduler(FileScheduler *file_scheduler);

private:
    void process_interactive_tasks();
};

#pragma once

#include <deque>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "task.h"
#include "filescheduler.h"


class Scheduler {
private:
    int id;
    FileScheduler *file_scheduler;
    std::deque<Task*> batch_task_queue;
    std::deque<Task*> async_response_task_queue;
    std::mutex mtx;
    std::atomic<bool> stop_flag;
    
public:
    Scheduler(int id);
    ~Scheduler();

    void submit(Task* task);
    void start();
    void stop();
    void setFileScheduler(FileScheduler *file_scheduler);
    void submit_async_response_task(Task* task);

private:
    void process_interactive_tasks();
};

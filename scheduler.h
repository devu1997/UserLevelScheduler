#pragma once

#include <deque>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include "task.h"
#include "priority_queue.h"
#include "calender_queue.h"
#include "filescheduler.h"


class Scheduler {
private:
    int id;
    FileScheduler *file_scheduler;
    PriorityQueue interactive_task_queue;
    CalenderQueue batch_task_queue;
    std::atomic<bool> stop_flag;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    
public:
    Scheduler(int id);
    ~Scheduler();

    void submit(Task* task);
    void start();
    void stop();
    void setFileScheduler(FileScheduler *file_scheduler);

private:
    void process_interactive_tasks();
    std::chrono::steady_clock::duration getDuration();
};

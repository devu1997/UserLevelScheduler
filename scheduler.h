#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include "cputask.h"
#include "iotask.h"

class Scheduler {
private:
    std::queue<CpuTask*> cpu_task_queue;
    std::mutex cpu_mtx;
    std::condition_variable cpu_cv;
    std::atomic<bool> stop_flag;

    std::priority_queue<IoTask*> io_task_queue;
    std::mutex io_mtx;
    std::condition_variable io_cv;
    
public:
    Scheduler();
    ~Scheduler();

    void submit(Task* task);
    void start();
    void stop();

private:
    void process_interactive_tasks();
    void process_io_tasks();
};

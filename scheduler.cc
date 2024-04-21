#include "scheduler.h"
#include <thread>

Scheduler::Scheduler() : stop_flag(false) {}

Scheduler::~Scheduler() {}

void Scheduler::submit(Task* task) {
    if (dynamic_cast<CpuTask*>(task)) {
        std::lock_guard<std::mutex> lock(cpu_mtx);
        cpu_task_queue.push(dynamic_cast<CpuTask*>(task));
        // cpu_cv.notify_one();
    } else if (dynamic_cast<IoTask*>(task)) {
        std::lock_guard<std::mutex> lock(io_mtx);
        io_task_queue.push(dynamic_cast<IoTask*>(task));
        // io_cv.notify_one();
    } else {
        std::cerr << "Unknown task type" << std::endl;
    }
}

void Scheduler::process_interactive_tasks() {
    while (!stop_flag) {
        CpuTask* task = nullptr;
        {
            std::unique_lock<std::mutex> lock(cpu_mtx);
            // cpu_cv.wait(lock, [this] { return !cpu_task_queue.empty() || stop_flag; });
            if (stop_flag) return;
            if (cpu_task_queue.empty()) continue;
            task = cpu_task_queue.front();
            cpu_task_queue.pop();
        }
        task->method(task->args);
        if (task->next_tasks.size() > 0) {
            for (Task* next_task : task->next_tasks) {
                if (task->pass_result) {
                    next_task->args.insert(next_task->args.begin(), task->id);
                }
                submit(next_task);
            }
        }
    }
}

void Scheduler::process_io_tasks() {
    while (!stop_flag) {
        IoTask* task = nullptr;
        {
            std::unique_lock<std::mutex> lock(io_mtx);
            // io_cv.wait(lock, [this] { return !io_task_queue.empty() || stop_flag; });
            if (stop_flag) return;
            if (io_task_queue.empty()) continue;
            task = io_task_queue.top();
            io_task_queue.pop();
        }
        auto current_time = std::chrono::system_clock::now();
        if (task->end_time > current_time) {
            std::chrono::duration<double> time_difference = task->end_time - current_time;
            std::this_thread::sleep_for(time_difference);
        }
        if (task->next_tasks.size() > 0) {
            for (Task* next_task : task->next_tasks) {
                submit(next_task);
            }
        }
    }
}

void Scheduler::start() {
    stop_flag = false;
    int max_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < max_threads - 1; ++i) {
        threads.emplace_back([this] { process_interactive_tasks(); });
    }
    threads.emplace_back([this] { process_io_tasks(); });
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void Scheduler::stop() {
    stop_flag = true;
    // cpu_cv.notify_all();
}

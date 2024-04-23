#include <thread>
#include "scheduler.h"
#include "filetasks.h"


Scheduler::Scheduler() : stop_flag(false) {}

Scheduler::~Scheduler() {}

void Scheduler::setFileScheduler(FileScheduler *file_scheduler) {
    this->file_scheduler = file_scheduler;
}

void Scheduler::submit(Task* task) {
    switch (task->exec_mode) {
        case TaskExecutionMode::SYNC:
            {
                std::lock_guard<std::mutex> lock(cpu_mtx);
                cpu_task_queue.push(task);
            }
            break;
        case TaskExecutionMode::ASYNC_FILE:
            {
                FileReadTask* file_task = static_cast<FileReadTask*>(task);
                std::lock_guard<std::mutex> lock(io_mtx);
                file_scheduler->submit(file_task);
            }
            break;
        default:
            break;
    }
}

void Scheduler::process_interactive_tasks() {
    while (!stop_flag) {
        Task* task = nullptr;
        {
            std::lock_guard<std::mutex> lock(cpu_mtx);
            if (stop_flag) return;
            if (cpu_task_queue.empty()) continue;
            task = cpu_task_queue.front();
            cpu_task_queue.pop();
        }
        try {
            auto result = task->process();
            if (task->next_tasks.size() > 0) {
                for (Task* next_task : task->next_tasks) {
                    if (task->forward_result) {
                        next_task->setInput(result);
                    }
                    submit(next_task);
                }
            }
        } catch (const std::runtime_error& e) {
            std::cout << "Caught runtime error: " << e.what() << std::endl;
        }
    }
}

void Scheduler::start() {
    this->process_interactive_tasks();
}

void Scheduler::stop() {
    stop_flag = true;
}

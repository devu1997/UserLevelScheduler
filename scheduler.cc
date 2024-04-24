#include <thread>
#include <chrono>
#include "scheduler.h"
#include "filetasks.h"


Scheduler::Scheduler(int id) : id(id), stop_flag(false) {}

Scheduler::~Scheduler() {
    if (file_scheduler) {
        delete file_scheduler;
    }
}

void Scheduler::setFileScheduler(FileScheduler *file_scheduler) {
    this->file_scheduler = file_scheduler;
}

void Scheduler::submit(Task* task) {
    std::cout << "Interactive Score: " << task->getInteractivityScore() << std::endl;
    switch (task->exec_mode) {
        case TaskExecutionMode::SYNC:
            batch_task_queue.push_back(task);
            break;
        case TaskExecutionMode::ASYNC_FILE:
            FileReadCompleteTask* file_complete_task = static_cast<FileReadCompleteTask*>(task);
            file_scheduler->submit(file_complete_task);
            break;
    }
}

void Scheduler::process_interactive_tasks() {
    while (!stop_flag) {
        file_scheduler->process_completed();
        if (batch_task_queue.empty()) continue; // steal
        Task* task = batch_task_queue.front();
        batch_task_queue.pop_front();

        auto start = std::chrono::steady_clock::now();
        auto result = task->process();
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        task->history.addEvent({EventType::CPU, duration});

        if (task->next_tasks.size() > 0) {
            for (Task* next_task : task->next_tasks) {
                if (task->forward_result) {
                    next_task->setInput(result);
                }
                next_task->setHistory(task->history);
                submit(next_task);
            }
        }
    }
}

void Scheduler::start() {
    this->file_scheduler = new FileScheduler();
    this->file_scheduler->setScheduler(this);
    this->process_interactive_tasks();
}

void Scheduler::stop() {
    stop_flag = true;
}

#include <thread>
#include "scheduler.h"
#include "filetasks.h"


Scheduler::Scheduler(int id) : id(id), stop_flag(false) {}

Scheduler::~Scheduler() {}

void Scheduler::setFileScheduler(FileScheduler *file_scheduler) {
    this->file_scheduler = file_scheduler;
}

void Scheduler::submit(Task* task) {
    switch (task->exec_mode) {
        case TaskExecutionMode::SYNC:
            batch_task_queue.push_back(task);
            break;
        case TaskExecutionMode::ASYNC_FILE:
            FileReadTask* file_task = static_cast<FileReadTask*>(task);
            FileReadCompleteTask* file_complete_task = new FileReadCompleteTask(id, file_task);
            file_scheduler->submit(file_complete_task);
            break;
    }
}

void Scheduler::submit_async_response_task(Task* task) {
    std::lock_guard<std::mutex> lock(mtx);
    async_response_task_queue.push_back(task);
}

void Scheduler::process_interactive_tasks() {
    while (!stop_flag) {
        if (stop_flag) return;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!async_response_task_queue.empty()) {
                batch_task_queue.insert(batch_task_queue.end(), std::make_move_iterator(async_response_task_queue.begin()), std::make_move_iterator(async_response_task_queue.end()));
                async_response_task_queue.clear();
            }
        }
        if (batch_task_queue.empty()) continue;
        Task* task = batch_task_queue.front();
        batch_task_queue.pop_front();
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

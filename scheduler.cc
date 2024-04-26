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

void Scheduler::setFileScheduler(FileScheduler* file_scheduler) {
    this->file_scheduler = file_scheduler;
}

std::chrono::steady_clock::duration Scheduler::getDuration() {
    auto current_time = std::chrono::steady_clock::now();
    return current_time - start_time;
}

void Scheduler::submit(Task* task) {
    int priority = task->getPriority();
    std::cout << "Interactive Score: " << task->getPriority() << std::endl;
    // task->updateCpuUtilization(getDuration(), 1);
    switch (task->exec_mode) {
        case TaskExecutionMode::SYNC:
            if (priority < PRI_MIN_BATCH) {
                interactive_task_queue.addTask(task, priority);
            } else {
                batch_task_queue.addTask(task, priority);
            }
            break;
        case TaskExecutionMode::ASYNC_FILE:
            if (dynamic_cast<AsyncFileReadTask*>(task)) {
                AsyncFileReadTask* async_task = dynamic_cast<AsyncFileReadTask*>(task);
                file_scheduler->submit(async_task);
            } else {
                AsyncFileWriteTask* async_task = dynamic_cast<AsyncFileWriteTask*>(task);
                file_scheduler->submit(async_task);
            }
            break;
    }
}

void Scheduler::process_interactive_tasks() {
    file_scheduler->process_completed();
    Task* task;
    if (interactive_task_queue.empty() && batch_task_queue.empty()) return; // steal
    if (!interactive_task_queue.empty()) {
        task = interactive_task_queue.getNextTask();
    } else {
        task = batch_task_queue.getNextTask();
    }
    // task->updateCpuUtilization(getDuration(), true);

    auto start = std::chrono::steady_clock::now();
    void* result = task->process();
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    task->history.addEvent({EventType::CPU, duration});

    if (task->next_tasks.size() > 0) {
        for (Task* next_task : task->next_tasks) {
            if (task->forward_result) {
                next_task->setInput(result);
            }
            next_task->setHistory(task->history);
            next_task->setNiceness(task->niceness);
            next_task->setTicks(task->ticks, task->ftick, task->ltick);
            submit(next_task);
        }
    }
}

void Scheduler::start() {
    this->file_scheduler = new FileScheduler();
    this->file_scheduler->setScheduler(this);
    while (!stop_flag) {
        this->process_interactive_tasks();
    }
}

void Scheduler::stop() {
    stop_flag = true;
}

#include <thread>
#include <chrono>
#include "scheduler.h"
#include "filetasks.h"


Scheduler::Scheduler(int id) : id(id), stop_flag(false) {
    current_task_count = 0;
    submitted_request_count = 0;
    completed_request_count = 0;
    this->file_scheduler = new FileScheduler();
}

Scheduler::~Scheduler() {
    if (file_scheduler) {
        delete file_scheduler;
    }
}

void Scheduler::setCoordinator(Coordinator* coordinator) {
    this->coordinator = coordinator;
}

double Scheduler::getCurrentTicks() {
    return (total_duration.count() * hz) / 1000;
}

void Scheduler::addToCurrentTicks(std::chrono::milliseconds duration) {
    total_duration = total_duration + duration;
}

void Scheduler::submit(Task* task) {
    int priority = task->getPriority();
    logger.trace("Interactive Score: %d Run time: %d Sleep time: %d", task->getPriority(), task->history.run_time.count(), task->history.sleep_time.count());
    switch (task->exec_mode) {
        case TaskExecutionMode::SYNC:
            if (priority < CALENDERQ_MIN_PRIORITY) {
                logger.trace("Interactive");
                interactive_task_queue.addTask(task, priority);
            } else {
                logger.trace("Batch");
                batch_task_queue.addTask(task, priority);
            }
            current_task_count++;
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

void Scheduler::submitToSubmissionQueue(int task_count, Scheduler* scheduler) {
    submission_queues[scheduler->id].enque({task_count, scheduler});
    submitted_request_count += task_count;
}

void Scheduler::submitToCompletionQueue(Task* task, Scheduler* scheduler) {
    completion_queues[scheduler->id].enque(task);
    completed_request_count++;
}

void Scheduler::process_interactive_tasks() {
    // Add completed IOs to runqueue
    file_scheduler->process_completed();

    // Donate tasks to steal
    if (submitted_request_count > 0) {
        for (auto &itr : submission_queues) {
            int scheduler_id = itr.first;
            while (!submission_queues[scheduler_id].empty()) {
                StealRequest ev = submission_queues[scheduler_id].deque();
                for (int i=0; i<current_task_count && i<ev.task_count; i++) {
                    Task* task;
                    if (!interactive_task_queue.empty()) {
                        task = interactive_task_queue.getNextTask();
                    } else {
                        task = batch_task_queue.getNextTask();
                    }
                    current_task_count--;
                    ev.scheduler->submitToCompletionQueue(task, this);
                    logger.trace("Donated");
                }
            }
        }
    }

    // Accept tasks donated to steal
    if (completed_request_count > 0) {
        for (auto &itr : completion_queues) {
            int scheduler_id = itr.first;
            while (!completion_queues[scheduler_id].empty()) {
                Task* task = completion_queues[scheduler_id].deque();
                submit(task);
                current_stealable_task_count--;
                logger.trace("Accepted donation");
            }
        }
    }

    // Request for tasks to steal
    if (interactive_task_queue.empty() && batch_task_queue.empty()) {
        if (current_stealable_task_count > 0) return;
        current_stealable_task_count = coordinator->stealTasks(this);
        if (current_stealable_task_count > 0) logger.trace("Requested to donate %d", current_stealable_task_count);
        return;
    }

    // Run tasks from task queue;
    Task* task;
    if (!interactive_task_queue.empty()) {
        task = interactive_task_queue.getNextTask();
    } else {
        task = batch_task_queue.getNextTask();
    }
    current_task_count--;

    logger.info("Scheduler %d running task %d", id, task->id);
    task->updateCpuUtilization(getCurrentTicks(), false);
    auto start = std::chrono::steady_clock::now();
    void* result = task->process();
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    task->history.addEvent({EventType::CPU, duration});
    addToCurrentTicks(duration);
    task->updateCpuUtilization(getCurrentTicks(), true);


    if (task->next_tasks.size() > 0) {
        for (Task* next_task : task->next_tasks) {
            if (task->forward_result) {
                next_task->setInput(result);
            }
            next_task->setHistory(task->history);
            next_task->setNiceness(task->niceness); // TODO: Have extra field to decide when to propogate niceness
            next_task->setTicks(task->ticks, task->ftick, task->ltick);
            submit(next_task);
        }
    }
    delete task;
}

void Scheduler::start() {
    this->file_scheduler->setScheduler(this);
    while (!stop_flag) {
        this->process_interactive_tasks();
    }
}

void Scheduler::stop() {
    stop_flag = true;
}

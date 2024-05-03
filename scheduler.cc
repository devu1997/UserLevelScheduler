#include <thread>
#include <chrono>
#include <fstream>
#include "scheduler.h"
#include "filetasks.h"


Scheduler::Scheduler(int id, int queue_size) : id(id), stop_flag(false), submission_queues(queue_size), completion_queues(queue_size) {
    current_task_count = 0;
    in_process_steal_requests = 0;
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

long long Scheduler::getCurrentTicks() {
    return (total_duration.count() * hz) / 1000;
}

void Scheduler::addToCurrentTicks(std::chrono::milliseconds duration) {
    total_duration = total_duration + duration;
}

void Scheduler::submit(Task* task) {
    int priority = task->getPriority();
    logger.trace("Group %s has Interactive Score: %d Run time: %d Sleep time: %d", task->group.c_str(), task->getPriority(), task->history.run_time.count(), task->history.sleep_time.count());
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
    submitted_request_count++;
    logger.trace("Self balancing by moving %d tasks from scheduler %d to %d", task_count, id, scheduler->id);
}

void Scheduler::submitToOwnerSubmissionQueue(int task_count, Scheduler* scheduler) {
    submission_queues[id].enque({task_count, scheduler});
    submitted_request_count++;
    logger.trace("Periodic balancing by moving %d tasks from scheduler %d to %d", task_count, id, scheduler->id);
}

void Scheduler::submitToCompletionQueue(Task* task, Scheduler* scheduler) {
    completion_queues[scheduler->id].enque(task);
    completed_request_count++;
}

void Scheduler::markStealRequestCompletion() {
    in_process_steal_requests--;
}

Task* Scheduler::getNextTask() {
    Task* task;
    if (!interactive_task_queue.empty()) {
        task = interactive_task_queue.getNextTask();
    } else {
        task = batch_task_queue.getNextTask();
    }
    current_task_count--;
    return task;
}

void Scheduler::process_interactive_tasks() {
    // Add completed IOs to runqueue
    file_scheduler->process_completed();

    // Donate tasks to steal
    if (submitted_request_count.load(std::memory_order_relaxed) > 0) {
        for (size_t index = 0; index < submission_queues.size(); ++index) {
            auto& submission_queue = submission_queues[index];
            while (!submission_queue.empty()) {
                StealRequest ev = submission_queue.deque();
                for (int i=0; i<current_task_count && i<ev.task_count; i++) {
                    Task* task = getNextTask();
                    ev.scheduler->submitToCompletionQueue(task, this);
                }
                submitted_request_count--;
                if (index != id) {
                    ev.scheduler->markStealRequestCompletion();
                }
            }
        }
    }

    // Accept tasks donated to steal
    if (completed_request_count.load(std::memory_order_relaxed) > 0) {
        for (auto &completion_queue : completion_queues) {
            while (!completion_queue.empty()) {
                Task* task = completion_queue.deque();
                submit(task);
                completed_request_count--;
            }
        }
    }

    // Request for tasks to steal
    if (interactive_task_queue.empty() && batch_task_queue.empty()) {
        if (in_process_steal_requests > 0 || completed_request_count > 0) return;
        int ret = coordinator->stealTasks(this);
        if (ret > 0) {
            in_process_steal_requests++;
        }
        return;
    }

    // Run tasks from task queue;
    Task* task = getNextTask();

    logger.trace("Scheduler %d running task %d", id, task->id);
    task->updateCpuUtilization(getCurrentTicks(), false);
    auto start = std::chrono::steady_clock::now();
    #ifdef ENABLE_INTERACTIVITY_METRICS
    penalities[task->group].push_back({start, task->getInteractivityPenality()});
    #endif
    void* result = task->process();
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    task->history.addEvent({EventType::CPU, duration});
    addToCurrentTicks(duration);
    task->updateCpuUtilization(getCurrentTicks(), true);

    #ifdef ENABLE_METRICS
    runtimes[task->group].push_back({end, duration.count()});
    #endif
    #ifdef ENABLE_LOAD_BALANCE_METRICS
    task_queue_sizes.push_back({end, current_task_count});
    #endif

    if (task->next_tasks.size() > 0) {
        for (Task* next_task : task->next_tasks) {
            next_task->inherit_from_parent(task, result);
            submit(next_task);
        }
    } else {
        logger.info("Task chain of group %s completed with id %d in scheduler %d in runtime %ld ms", task->group.c_str(), task->id, id, task->ticks / hz);
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

    #ifdef ENABLE_METRICS
    logger.info("Running metrics collection in scheduler %d", id);
    for (auto &group_runtimes_pair : runtimes) {
        std::string group = group_runtimes_pair.first;
        std::ofstream out_file;
        out_file.open("./results/latencies_" + std::to_string(id) + "_" + group + ".csv"); 
        for (auto &itr : group_runtimes_pair.second) {
            out_file << (std::chrono::duration_cast<std::chrono::milliseconds>(itr.first - steady_now)).count() << " " << itr.second << ",";
        }
        out_file << std::endl;
        out_file.close();
    }
    #endif
    #ifdef ENABLE_INTERACTIVITY_METRICS
    logger.info("Running interactive metrics collection in scheduler %d", id);
    for (auto &group_penality_pair : penalities) {
        std::string group = group_penality_pair.first;
        std::ofstream out_file;
        out_file.open("./results/penalities_" + std::to_string(id) + "_" + group + ".csv"); 
        for (auto &itr : group_penality_pair.second) {
            out_file << (std::chrono::duration_cast<std::chrono::milliseconds>(itr.first - steady_now)).count() << " " << itr.second << ",";
        }
        out_file << std::endl;
        out_file.close();
    }
    #endif
    #ifdef ENABLE_LOAD_BALANCE_METRICS
    logger.info("Running load balance metrics collection in scheduler %d", id);
    std::ofstream out_file;
    out_file.open("./results/queue_sizes_" + std::to_string(id) + ".csv"); 
    for (auto &itr : task_queue_sizes) {
        out_file << (std::chrono::duration_cast<std::chrono::milliseconds>(itr.first - steady_now)).count() << " " << itr.second << ",";
    }
    out_file << std::endl;
    out_file.close();
    #endif
}

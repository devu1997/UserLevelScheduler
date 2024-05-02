#pragma once

#include <deque>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include "task.h"
#include "priority_queue.h"
#include "calender_queue.h"
#include "producer_consumer_queue.h"
#include "filescheduler.h"
#include "logger.h"


class Coordinator;

struct StealRequest {
    int task_count;
    Scheduler* scheduler;
};

class Scheduler {
private:
    Coordinator* coordinator;
    FileScheduler* file_scheduler;
    PriorityQueue interactive_task_queue;
    CalenderQueue batch_task_queue;
    std::atomic<bool> stop_flag;
    std::chrono::milliseconds total_duration = std::chrono::milliseconds(0);

    std::atomic<int> in_process_steal_requests;
    std::atomic<int> submitted_request_count;
    std::atomic<int> completed_request_count;
    std::vector<ProducerConsumerQueue<StealRequest>> submission_queues;
    std::vector<ProducerConsumerQueue<Task*>> completion_queues;

    #ifdef ENABLE_METRICS
    double cum_cpu_runtime = 0;
    double cum_io_runtime = 0;
    std::vector<std::pair<std::chrono::steady_clock::time_point, double>> cpu_task_runtimes;
    std::vector<std::pair<std::chrono::steady_clock::time_point, double>> io_task_runtimes;
    std::chrono::steady_clock::time_point steady_now = std::chrono::steady_clock::now();
    #endif
    
public:
    int id;
    std::atomic<int> current_task_count;

    Scheduler(int id, int queue_size);
    ~Scheduler();

    void submit(Task* task);
    void start();
    void stop();
    void setCoordinator(Coordinator* coordinator);
    void submitToSubmissionQueue(int task_count, Scheduler* scheduler);
    void submitToOwnerSubmissionQueue(int task_count, Scheduler* scheduler);
    void submitToCompletionQueue(Task* task, Scheduler* scheduler);
    void markStealRequestCompletion();
    Task* getNextTask();

private:
    void process_interactive_tasks();
    long long getCurrentTicks();
    void addToCurrentTicks(std::chrono::milliseconds duration);
};

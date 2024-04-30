#pragma once

#include <deque>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include "task.h"
#include "priority_queue.h"
#include "calender_queue.h"
#include "ring_buffer.h"
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

    int current_stealable_task_count = 0;
    std::atomic<int> submitted_request_count;
    std::atomic<int> completed_request_count;
    std::unordered_map<int, RingBuffer<StealRequest>> submission_queues;
    std::unordered_map<int, RingBuffer<Task*>> completion_queues;
    
public:
    int id;
    std::atomic<int> current_task_count;

    Scheduler(int id);
    ~Scheduler();

    void submit(Task* task);
    void start();
    void stop();
    void setCoordinator(Coordinator* coordinator);
    void submitToSubmissionQueue(int task_count, Scheduler* scheduler);
    void submitToCompletionQueue(Task* task, Scheduler* scheduler);

private:
    void process_interactive_tasks();
    double getCurrentTicks();
    void addToCurrentTicks(std::chrono::milliseconds duration);
};

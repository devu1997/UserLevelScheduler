#include <stdexcept>
#include "calender_queue.h"

CalenderQueue::CalenderQueue() : queue(NQUEUE), size_(0), runq(0), insq(1), last_insq_update_time(std::chrono::steady_clock::now()) {}

void CalenderQueue::addTask(Task* task, int priority) {
    auto current_time = std::chrono::steady_clock::now();
    int increments = std::chrono::milliseconds(INSQ_UPDATE_INTERVAL) / (current_time - last_insq_update_time);
    insq = (insq + increments) % NQUEUE;
    if (runq == insq) {
        insq = (runq + 1) % NQUEUE;
    }
    int insert_position = (insq + priority - PRI_MIN_BATCH) % NQUEUE;
    queue[insert_position].push_back(task);
    size_++;
}

Task* CalenderQueue::getNextTask() {
    if (size_ == 0) {
        throw std::runtime_error("Error: Invalid access to empty calender queue");
    }
    while (true) {
        if (queue[runq].empty()) {
            runq = (runq + 1) % NQUEUE;
            if (runq == insq) {
                insq = (runq + 1) % NQUEUE;
            }
            continue;
        }
        Task* task = queue[runq].front();
        queue[runq].pop_front();
        size_--;
        return task;
    }
}

bool CalenderQueue::empty() {
  return size_ == 0;
}


size_t CalenderQueue::size() {
  return size_;
}
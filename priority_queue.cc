#include <stdexcept>
#include "priorities.h"
#include "priority_queue.h"

PriorityQueue::PriorityQueue() : queue(PRIORITYQ_NQUEUE), bitmap(PRIORITYQ_NQUEUE), size_(0) {}

void PriorityQueue::addTask(Task* task, int priority) {
    priority = priority - PRIORITYQ_MIN_PRIORITY;
    queue[priority].push_back(task);
    size_++;
    bitmap[priority] = 1;
}

Task* PriorityQueue::getNextTask() {
    for (int priority = 0; priority < PRIORITYQ_NQUEUE; priority++) {
        if (bitmap[priority]) {
            Task* task = queue[priority].front();
            queue[priority].pop_front();
            size_--;
            if (queue[priority].empty()) {
                bitmap[priority] = 0;
            }
            return task;
        }
    }
    throw std::runtime_error("Error: Invalid access to empty priority queue");
}

bool PriorityQueue::empty() {
  return size_ == 0;
}


size_t PriorityQueue::size() {
  return size_;
}
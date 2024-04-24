#pragma once

#include <queue>
#include <deque>
#include "task.h"


class PriorityQueue {
private:
    std::vector<std::deque<Task*>> queue;
    size_t size_;
    std::vector<bool> bitmap;

public:
    PriorityQueue();

    void addTask(Task* task);
    Task* getNextTask();
    bool empty();
    size_t size();
};
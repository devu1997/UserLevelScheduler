#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <any>


class Task {
private:
    static int last_task_id;

public:
    int id;
    void* input;
    bool forward_result;
    std::vector<Task*> next_tasks;

    Task(bool forward_result = false);

    virtual void* process() = 0;
    void setInput(void* input);
    void setNextTasks(std::vector<Task*> &next_tasks);

    static int generate_task_id();
};

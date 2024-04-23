#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <any>

enum class TaskExecutionMode {
    SYNC,
    ASYNC_FILE
};

class Task {
private:
    static int last_task_id;

public:
    int id;
    TaskExecutionMode exec_mode; 
    void* input;
    bool forward_result;
    std::vector<Task*> next_tasks;

    Task(bool forward_result = false, TaskExecutionMode exec_mode = TaskExecutionMode::SYNC);

    virtual void* process() = 0;
    void setInput(void* input);
    void setNextTasks(std::vector<Task*> &next_tasks);

    static int generate_task_id();
};

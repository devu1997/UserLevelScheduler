#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <any>
#include "history.h"

enum class TaskExecutionMode {
    SYNC,
    ASYNC_FILE
};

class Task {
private:
    static int last_task_id;

public:
    int id;
    void* input;
    bool forward_result;
    std::vector<Task*> next_tasks;
    TaskExecutionMode exec_mode;
    History history;

    Task(bool forward_result = false, TaskExecutionMode exec_mode = TaskExecutionMode::SYNC);

    virtual void* process() = 0;
    void setInput(void* input);
    void setNextTasks(std::vector<Task*> &next_tasks);
    void setHistory(History history);
    int getInteractivityScore();

    static int generate_task_id();
};

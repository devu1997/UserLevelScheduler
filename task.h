#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <any>
#include <functional>
#include "history.h"
#include "priorities.h"


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
    std::vector<Task*> next_tasks;
    History history;
    bool forward_result = true;
    TaskExecutionMode exec_mode = TaskExecutionMode::SYNC;
    int niceness = DEFAULT_NICENESS;
    long ticks = 0;
    long ftick = 0;
    long ltick = 0;

    Task();

    virtual void* process() = 0;
    virtual Task* fork() = 0;

    void setInput(void* input);
    void setForwardResult(bool forward_result);
    void setNextTasks(std::vector<Task*> next_tasks);
    void setHistory(History history);
    void setNiceness(int niceness);
    void setTicks(long ticks, long ftick, long ltick);
    void setExecutionMode(TaskExecutionMode exec_mode);
    void copy(Task* task);

    int getInteractivityPenality();
    int getPriority();
    void updateCpuUtilization(double duration, bool run);

    static int generate_task_id();
};

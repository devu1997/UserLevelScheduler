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
    bool inherit_niceness = true;
    bool inherit_result = true;

public:
    int id;
    void* input;
    std::vector<Task*> next_tasks;
    bool forward_result = true;
    History history;
    TaskExecutionMode exec_mode = TaskExecutionMode::SYNC;
    int niceness = DEFAULT_NICENESS;
    unsigned long long ticks = 0;
    unsigned long long ftick = 0;
    unsigned long long ltick = 0;
    std::string group = "GRP0";

    Task();

    virtual void* process() = 0;
    virtual Task* fork() = 0;

    void setInput(void* input);
    void setForwardResult(bool forward_result);
    void setNextTasks(std::vector<Task*> next_tasks);
    void setHistory(History history);
    void setNiceness(int niceness);
    void setTicks(unsigned long long ticks, unsigned long long ftick, unsigned long long ltick);
    void setExecutionMode(TaskExecutionMode exec_mode);
    void setGroup(std::string group);
    void inherit_from_parent(Task* parent_task, void* parent_result);
    void copy(Task* task);

    int getInteractivityPenality();
    int getPriority();
    void updateCpuUtilization(unsigned long long total_ticks, bool run);

    static int generate_task_id();
};

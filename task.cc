#include "task.h"


#define SCALING_FACTOR 50
#define INTERACTIVITY_THRESHOLD 30

int Task::last_task_id = 0;

Task::Task(bool forward_result, TaskExecutionMode exec_mode) {
    this->id = Task::generate_task_id();
    this->forward_result = forward_result;
    this->exec_mode = exec_mode;
}

void Task::setInput(void* input) {
    this->input = input;
}

void Task::setNextTasks(std::vector<Task*> &next_tasks) {
    this->next_tasks = next_tasks;
}

void Task::setHistory(History history) {
    this->history = history;
}

int Task::getInteractivityScore() {
    if (history.sleep_time == std::chrono::nanoseconds::zero() && history.run_time == std::chrono::nanoseconds::zero()) {
        return SCALING_FACTOR; // Initially set the task as batch so that non-interactive tasks will not starve interactive tasks
    }
    int score = 0;
    if (history.sleep_time > history.run_time) {
        score = (SCALING_FACTOR * history.run_time) / history.sleep_time;
    } else {
        score = SCALING_FACTOR + ((SCALING_FACTOR * history.sleep_time) / history.run_time);
    }
    return score;
}

int Task::generate_task_id() {
    return ++last_task_id;
}

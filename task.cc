#include "task.h"


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

int Task::generate_task_id() {
    return ++last_task_id;
}

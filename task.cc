#include "task.h"

int Task::last_task_id = 0;

Task::Task(
  std::vector<std::any> args, 
  bool pass_result, 
  std::vector<Task*> next_tasks) {
    this->id = generate_task_id();
    this->args = args;
    this->pass_result = pass_result;
    this->next_tasks = next_tasks;
  }

int Task::generate_task_id() {
    return ++last_task_id;
}

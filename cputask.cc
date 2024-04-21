#include "cputask.h"

CpuTask::CpuTask(
  std::function<std::any(std::vector<std::any>)> method,
  std::vector<std::any> args, 
  bool pass_result, 
  std::vector<Task*> next_tasks) : Task(args, pass_result, next_tasks) {
    this->method = method;
  }

#include "cputask.h"


CpuTask::CpuTask(std::function<void*(void*)> func) : Task::Task() {
    this->func = func;
}

void* CpuTask::process() {
  return func(this->input);
}

Task* CpuTask::fork() {
    CpuTask* task = new CpuTask(this->func);
    this->copy(task);
    return task;
}

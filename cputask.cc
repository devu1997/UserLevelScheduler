#include "cputask.h"


void* CpuTask::process() {
  return func();
}

Task* CpuTask::fork() {
    CpuTask* task = new CpuTask(this->func);
    this->copy(task);
    return task;
}

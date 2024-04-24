#include "cputask.h"

CpuTask::CpuTask(std::function<void*()> func, bool forward_result) : Task(func, forward_result, TaskExecutionMode::SYNC) {}

void* CpuTask::process() {
  return func();
}

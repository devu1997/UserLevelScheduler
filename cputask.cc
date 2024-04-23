#include "cputask.h"

CpuTask::CpuTask(bool forward_result) : Task(forward_result, TaskExecutionMode::SYNC) {}

void* CpuTask::process() {
  CpuTaskInput* cpu_input = static_cast<CpuTaskInput*>(input);
  return cpu_input->func(cpu_input->args);
}

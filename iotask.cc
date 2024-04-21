#include "iotask.h"
#include <chrono>

IoTask::IoTask(
  int time_in_microseconds, 
  bool pass_result, 
  std::vector<Task*> next_tasks) : Task({}, pass_result, next_tasks) {
    auto current_time = std::chrono::system_clock::now();
    this->end_time = current_time + std::chrono::microseconds(time_in_microseconds);
}

bool IoTask::operator<(const IoTask& other) const {
    return end_time < other.end_time;
}

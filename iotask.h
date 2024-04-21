#pragma once

#include "task.h"

class IoTask : public Task {
    
public:
    std::chrono::time_point<std::chrono::system_clock> end_time;

    IoTask(
      int time_in_microseconds, 
      bool pass_result = false, 
      std::vector<Task*> next_tasks = {});

    bool operator<(const IoTask& other) const;
};

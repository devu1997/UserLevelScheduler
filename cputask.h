#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <any>
#include "task.cc"

class CpuTask : public Task {

public:
    int id;
    std::function<std::any(std::vector<std::any>)> method;

    CpuTask(
      std::function<std::any(std::vector<std::any>)> method = [](std::vector<std::any> args) -> std::any { return std::any(); },
      std::vector<std::any> args = {}, 
      bool pass_result = false, 
      std::vector<Task*> next_tasks = {});
    
};

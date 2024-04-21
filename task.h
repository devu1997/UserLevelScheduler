#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <any>

class Task {
private:
    static int last_task_id;

public:
    int id;
    std::vector<std::any> args;
    bool pass_result;
    std::vector<Task*> next_tasks;

    Task(
      std::vector<std::any> args,
      bool pass_result = false, 
      std::vector<Task*> next_tasks = {});
    virtual ~Task() {}
    
    static int generate_task_id();
};

#include <iostream>
#include "scheduler.cc"
#include "cputask.cc"
#include "iotask.cc"

int main() {
    Scheduler scheduler;

    std::vector<std::any> args1 = { std::string("Running task 1\n") };
    CpuTask *task1 = new CpuTask([](std::vector<std::any> args) -> std::any { 
      std::cout << std::any_cast<std::string>(args[0]);
      return std::any(); 
    }, args1);

    std::vector<std::any> args2 = { std::string("Running task 2\n") };
    CpuTask *task2 = new CpuTask([](std::vector<std::any> args) -> std::any { 
      std::cout << std::any_cast<std::string>(args[0]);
      return std::any(); 
    }, args2);

    // std::vector<std::any> args3 = { std::string("Running task 3\n") };
    // CpuTask *task3 = new CpuTask([](std::vector<std::any> args) -> std::any { 
    //   std::cout << std::any_cast<std::string>(args[0]);
    //   return std::any(); 
    // }, args3, false, {task2});


    IoTask *task4 = new IoTask(2000000, false, {task2});

    scheduler.submit(task1);
    // scheduler.submit(task3);
    scheduler.submit(task4);

    scheduler.start();

    return 0;
}
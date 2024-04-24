#include <iostream>
#include "history.cc"
#include "task.cc"
#include "cputask.cc"
#include "filetasks.cc"
#include "scheduler.cc"
#include "filescheduler.cc"
#include "coordinator.cc"
#include <fcntl.h>
#include <thread>
#include <chrono>


int main() {
    Coordinator coordinator;

    CpuTask *task1 = new CpuTask(
        []() -> void* { 
          std::cout << "Running task 1\n";
          return nullptr;
        }
      );

    CpuTask *task2 = new CpuTask(
        []() -> void* { 
          std::cout << "Running task 2\n";
          return nullptr;
        }
      );

    CpuTask *task3 = new CpuTask(
        []() -> void* { 
          std::cout << "Running task 3\n";
          return nullptr;
        }
      );
    std::vector<Task*> next_tasks = {task2};
    task3->setNextTasks(next_tasks);

    FileOpenTask *fo = new FileOpenTask();
    fo->setInput(new FileOpenTaskInput({"/home/users/devika/os/custom-project/log.txt", O_RDONLY}));
    FileReadTask *fr = new FileReadTask();
    FileCloseTask *fc = new FileCloseTask(false);

    CpuTask *task4 = new CpuTask(
      []() -> void* { 
        std::cout << "Running task 4\n";
        long long result = 0;
        for (int i = 0; i < 10000000; ++i) {
            result += i * i; // Perform some arithmetic operations
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "Result of CPU-intensive task: " << result << std::endl;
        return nullptr;
      }
    );
    std::vector<Task*> next_tasks0 = {fo};
    std::vector<Task*> next_tasks1 = {fr};
    std::vector<Task*> next_tasks2 = {fc};
    std::vector<Task*> next_tasks3 = {task3};
    task4->setNextTasks(next_tasks0);
    fo->setNextTasks(next_tasks1);
    fr->setNextTasks(next_tasks2);
    fc->setNextTasks(next_tasks3);

    
    coordinator.submit(task1);
    coordinator.submit(task4);
    coordinator.submit(task3);
    // coordinator.submit(task3);
    coordinator.submit(task1);
    // coordinator.submit(task3);
    // coordinator.submit(fo);

    coordinator.start();

    return 0;
}
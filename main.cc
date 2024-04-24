#include <iostream>
#include "history.cc"
#include "task.cc"
#include "cputask.cc"
#include "filetasks.cc"
#include "priority_queue.cc"
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
          // long long result = 0;
          // for (int i = 0; i < 10000000; ++i) {
          //     result += i * i; // Perform some arithmetic operations
          // }
          // std::this_thread::sleep_for(std::chrono::seconds(10));
          // std::cout << "Result of CPU-intensive task: " << result << std::endl;
          return nullptr;
        }
      );

    CpuTask *task4 = new CpuTask(
      []() -> void* { 
        std::cout << "Running task 4\n";
        // long long result = 0;
        // for (int i = 0; i < 10000000; ++i) {
        //     result += i * i; // Perform some arithmetic operations
        // }
        // std::cout << "Result of CPU-intensive task: " << result << std::endl;
        try {
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        } catch (const std::runtime_error& e) {
                std::cout << "Caught runtime error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Caught exception: " << e.what() << std::endl;
            } catch (...) {
                std::cout << "Caught unknown exception" << std::endl;
            } 
        std::cout << "Running task 4\n";
        return nullptr;
      }
    );

    FileOpenTask *fo = new FileOpenTask();
    fo->setInput(new FileOpenTaskInput({"/home/users/devika/os/custom-project/log.txt", O_RDONLY}));
    FileReadTask *fr = new FileReadTask();
    FileCloseTask *fc = new FileCloseTask(false);
    
    task4->setNextTasks({fo});
    fo->setNextTasks({fr});
    fr->setNextTasks({fc});
    fc->setNextTasks({task3});
    task3->setNextTasks({task2});
    
    coordinator.submit(task1);
    coordinator.submit(task4);
    coordinator.submit(task3);
    // coordinator.submit(task1);

    coordinator.start();

    return 0;
}
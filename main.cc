#include <iostream>
#include "history.cc"
#include "task.cc"
#include "cputask.cc"
#include "filetasks.cc"
#include "priority_queue.cc"
#include "calender_queue.cc"
#include "scheduler.cc"
#include "filescheduler.cc"
#include "coordinator.cc"
#include <fcntl.h>
#include <thread>
#include <chrono>

int task_counter = 0;

int main() {
    Coordinator coordinator;

    Task *task1 = new CpuTask(
        []() -> void* { 
          std::cout << "Running task " << ++task_counter << "\n";
          try {
              std::this_thread::sleep_for(std::chrono::milliseconds(2000));
          } catch (const std::runtime_error& e) {
              std::cout << "Caught runtime error: " << e.what() << std::endl;
          } catch (const std::exception& e) {
              std::cout << "Caught exception: " << e.what() << std::endl;
          } catch (...) {
              std::cout << "Caught unknown exception" << std::endl;
          } 
          return nullptr;
        }
      );
    task1->setForwardResult(false);

    FileOpenTask *fo = new FileOpenTask();
    fo->setInput(new FileOpenTaskInput({"/home/users/devika/os/custom-project/log.txt", O_RDONLY}));
    FileReadTask *fr = new FileReadTask();
    FileCloseTask *fc = new FileCloseTask();
    
    Task* start = task1->fork();
    Task* task1_fork = start;
    for (int i=0; i<10; i++) {
      Task* fo_fork = fo->fork();
      Task* fr_fork = fr->fork();
      Task* fc_fork = fc->fork();
      task1_fork->setNextTasks({fo_fork});
      fo_fork->setNextTasks({fr_fork});
      fr_fork->setNextTasks({fc_fork});
      task1_fork = task1->fork();
      fc_fork->setNextTasks({task1_fork});
    }
    
    coordinator.submit(start);
    coordinator.start();

    return 0;
}
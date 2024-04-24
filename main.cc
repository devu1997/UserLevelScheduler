#include <iostream>
#include "task.cc"
#include "cputask.cc"
#include "filetasks.cc"
#include "scheduler.cc"
#include "filescheduler.cc"
#include "coordinator.cc"
#include <fcntl.h>


int main() {
    Coordinator coordinator;

    CpuTask *task1 = new CpuTask();
    task1->setInput(
      new CpuTaskInput({
        []() -> void* { 
          std::cout << "Running task 1\n";
          return nullptr;
        }
      }));

    CpuTask *task2 = new CpuTask();
    task2->setInput(
      new CpuTaskInput({
        []() -> void* { 
          std::cout << "Running task 2\n";
          return nullptr;
        }
      }));

    CpuTask *task3 = new CpuTask();
    task3->setInput(
      new CpuTaskInput({
        []() -> void* { 
          std::cout << "Running task 3\n";
          return nullptr;
        }
      }));
    std::vector<Task*> next_tasks = {task2};
    task3->setNextTasks(next_tasks);

    FileOpenTask *fo = new FileOpenTask();
    fo->setInput(new FileOpenTaskInput({"/home/users/devika/os/custom-project/log.txt", O_RDONLY}));
    FileReadTask *fr = new FileReadTask();
    FileCloseTask *fc = new FileCloseTask();

    std::vector<Task*> next_tasks1 = {fr};
    std::vector<Task*> next_tasks2 = {fc};
    fo->setNextTasks(next_tasks1);
    fr->setNextTasks(next_tasks2);

    
    coordinator.submit(task1);
    coordinator.submit(task1);
    coordinator.submit(fo);
    coordinator.submit(task3);
    coordinator.submit(task3);
    coordinator.submit(task1);
    coordinator.submit(task3);

    coordinator.start();

    return 0;
}
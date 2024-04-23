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

    std::vector<std::any> args1 = { std::string("Running task 1\n") };
    CpuTask *task1 = new CpuTask();
    task1->setInput(
      new CpuTaskInput({
        args1,
        [](std::vector<std::any> args) -> void* { 
          std::cout << std::any_cast<std::string>(args[0]);
          return nullptr;
        }
      }));

    std::vector<std::any> args2 = { std::string("Running task 2\n") };
    CpuTask *task2 = new CpuTask();
    task2->setInput(
      new CpuTaskInput({
        args2,
        [](std::vector<std::any> args) -> void* { 
          std::cout << std::any_cast<std::string>(args[0]);
          return nullptr;
        }
      }));

    std::vector<std::any> args3 = { std::string("Running task 3\n") };
    CpuTask *task3 = new CpuTask();
    task3->setInput(
      new CpuTaskInput({
        args3,
        [](std::vector<std::any> args) -> void* { 
          std::cout << std::any_cast<std::string>(args[0]);
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

    
    coordinator.submit(fo);
    coordinator.submit(task1);
    coordinator.submit(task3);

    coordinator.start();

    return 0;
}
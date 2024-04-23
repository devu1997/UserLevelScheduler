#include <iostream>
#include "task.cc"
#include "cputask.cc"
#include "filetasks.cc"
#include "scheduler.cc"
#include "fileioscheduler.cc"
#include <fcntl.h>

void start(Scheduler &scheduler, FileScheduler &file_scheduler) {
    // stop_flag = false;
    int max_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < max_threads - 2; ++i) {
        threads.emplace_back([&scheduler] { scheduler.start(); });
    }
    threads.emplace_back([&scheduler, &file_scheduler] { file_scheduler.start_consumer(); });
    threads.emplace_back([&scheduler, &file_scheduler] { file_scheduler.start_producer(); });
    for (std::thread& thread : threads) {
        thread.join();
    }
}

int main() {
    Scheduler scheduler;
    FileScheduler file_scheduler;
    scheduler.setFileScheduler(&file_scheduler);
    file_scheduler.setScheduler(&scheduler);

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

    
    scheduler.submit(task1);
    scheduler.submit(task3);
    scheduler.submit(fo);

    start(scheduler, file_scheduler);

    return 0;
}
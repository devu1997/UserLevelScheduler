#include <thread>
#include "scheduler.h"


Scheduler::Scheduler() : stop_flag(false) {}

Scheduler::~Scheduler() {}

void Scheduler::submit(Task* task) {
    std::lock_guard<std::mutex> lock(cpu_mtx);
    cpu_task_queue.push(task);
}

void Scheduler::process_interactive_tasks() {
    while (!stop_flag) {
        Task* task = nullptr;
        {
            std::lock_guard<std::mutex> lock(cpu_mtx);
            if (stop_flag) return;
            if (cpu_task_queue.empty()) continue;
            task = cpu_task_queue.front();
            cpu_task_queue.pop();
        }
        try {
            std::cout << "HI" << std::endl;
            auto result = task->process();
            std::cout << "NEXT " << task->next_tasks.size() << " Size" << std::endl;
            if (task->next_tasks.size() > 0) {
                for (Task* next_task : task->next_tasks) {
                    if (task->forward_result) {
                        next_task->setInput(result);
                    }
                    submit(next_task);
                }
            }
        } catch (const std::runtime_error& e) {
            std::cout << "Caught runtime error: " << std::endl;
            // std::cout << "Caught runtime error: " << e.what() << std::endl;
        }
    }
}

// void Scheduler::process_simulated_io_tasks() {
//     while (!stop_flag) {
//         IoTask* task = nullptr;
//         {
//             std::unique_lock<std::mutex> lock(io_mtx);
//             // io_cv.wait(lock, [this] { return !io_task_queue.empty() || stop_flag; });
//             if (stop_flag) return;
//             if (io_task_queue.empty()) continue;
//             task = io_task_queue.top();
//             io_task_queue.pop();
//         }
//         auto current_time = std::chrono::system_clock::now();
//         if (task->end_time > current_time) {
//             std::chrono::duration<double> time_difference = task->end_time - current_time;
//             std::this_thread::sleep_for(time_difference);
//         }
//         if (task->next_tasks.size() > 0) {
//             for (Task* next_task : task->next_tasks) {
//                 submit(next_task);
//             }
//         }
//     }
// }

// void Scheduler::start() {
//     stop_flag = false;
//     int max_threads = std::thread::hardware_concurrency();
//     std::vector<std::thread> threads;
//     for (int i = 0; i < max_threads - 1; ++i) {
//         threads.emplace_back([this] { process_interactive_tasks(); });
//     }
//     FileScheduler *file_scheduler = new FileScheduler();
//     threads.emplace_back([this] { file_scheduler.process(this); });
//     for (std::thread& thread : threads) {
//         thread.join();
//     }
// }

void Scheduler::stop() {
    stop_flag = true;
}

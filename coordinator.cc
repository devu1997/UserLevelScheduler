#include <thread>
#include <cstring>
#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <execinfo.h>
#include "coordinator.h"

void signalHandler(int signal) {
    logger.error("Error: Segmentation fault (signal %d)", signal);

    // Print stack trace
    constexpr int maxFrames = 64;
    void* callstack[maxFrames];
    int numFrames = backtrace(callstack, maxFrames);
    backtrace_symbols_fd(callstack, numFrames, STDERR_FILENO);

    // Terminate the program
    std::exit(EXIT_FAILURE);
}

Coordinator::Coordinator() {
    int max_threads = std::thread::hardware_concurrency();
    for (int i = 0; i < max_threads; ++i) {
        this->schedulers.push_back(new Scheduler(i));
    }
}

Coordinator::~Coordinator() {
    while (schedulers.size() > 0) {
        Scheduler* scheduler = schedulers.back();
        schedulers.pop_back();
        delete scheduler;
    }
}

int Coordinator::stealTasks(Scheduler* scheduler) {
    int total_tasks = 0;
    int max_tasks = 0;
    Scheduler* max_tasks_scheduler;
    for (auto &curr_scheduler : schedulers) {
        if (curr_scheduler->id == scheduler->id) continue;
        total_tasks = total_tasks + curr_scheduler->current_task_count;
        if (max_tasks < curr_scheduler->current_task_count) {
            max_tasks = curr_scheduler->current_task_count;
            max_tasks_scheduler = curr_scheduler;
        }
    }
    int stealable_task_count = std::min(MAX_STEAL_TASKS, max_tasks - (int)std::ceil(((total_tasks * 1.0) / schedulers.size())));
    if (stealable_task_count > 0) {
        max_tasks_scheduler->submitToSubmissionQueue(stealable_task_count, scheduler);
    }
    return stealable_task_count;
}

void Coordinator::submit(Task* task) {
    schedulers[next_scheduler_id]->submit(task);
    next_scheduler_id = (next_scheduler_id + 1) % schedulers.size();
}

void Coordinator::start() {
    #ifdef ENABLE_PINNING
    #ifdef __linux__
    logger.info("Thread pinning enabled")
    #endif
    #endif

    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (auto &scheduler : schedulers) {
        scheduler->setCoordinator(this);
        threads.emplace_back([&scheduler, core_to_run] {
            std::signal(SIGSEGV, signalHandler);
            try {
                // Set CPU affinity
                #ifdef ENABLE_PINNING
                #ifdef __linux__
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);       
                CPU_SET(core_to_run, &cpuset);
                if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                    throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
                }
                #endif
                #endif
                scheduler->start(); 
            } catch (const std::runtime_error& e) {
                logger.error("Caught runtime error: %s", e.what());
            } catch (const std::exception& e) {
                logger.error("Caught exception: %s", e.what());
            } catch (...) {
                logger.error("Caught unknown exception");
            }
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void Coordinator::stop() {
    for (auto &scheduler : schedulers) {
        scheduler->stop();
    }
}
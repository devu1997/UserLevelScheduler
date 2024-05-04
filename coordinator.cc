#include <thread>
#include <cstring>
#include <cerrno>
#include <memory>
#include <chrono>
#include <random>
#include <cmath>
#include <fstream>
#include "coordinator.h"


Coordinator::Coordinator() {
    int max_threads = std::thread::hardware_concurrency() / 2;
    logger.info("Creating %d schedulers", max_threads);
    for (int i = 0; i < max_threads; ++i) {
        this->schedulers.push_back(new Scheduler(i, max_threads));
    }
}

Coordinator::~Coordinator() {
    while (schedulers.size() > 0) {
        Scheduler* scheduler = schedulers.back();
        schedulers.pop_back();
        delete scheduler;
    }
}

int Coordinator::stealTasks(Scheduler* recipient_scheduler) {
    int total_tasks = 0;
    int doner_current_task_count = 0;
    Scheduler* doner_scheduler;
    for (auto &scheduler : schedulers) {
        int current_task_count = scheduler->current_task_count;
        if (scheduler->id == recipient_scheduler->id) continue;
        total_tasks = total_tasks + current_task_count;
        if (doner_current_task_count < current_task_count) {
            doner_current_task_count = current_task_count;
            doner_scheduler = scheduler;
        }
    }
    int average_tasks_floor = total_tasks / schedulers.size();
    int average_tasks_ceil = std::ceil(((total_tasks * 1.0) / schedulers.size()));
    int max_acceptable_donations = average_tasks_floor;
    int curr_donated_tasks = std::min(max_acceptable_donations, doner_current_task_count - average_tasks_ceil);
    curr_donated_tasks = std::min(curr_donated_tasks, MAX_TASKS_TO_STEAL);

    if (curr_donated_tasks > 0) {
        doner_scheduler->submitToSubmissionQueue(curr_donated_tasks, recipient_scheduler);
        logger.trace("Self balancing total_tasks: %d, average_tasks_ceil: %d, average_tasks_floor: %d, curr_donated_tasks: %d", total_tasks, average_tasks_ceil, average_tasks_floor, curr_donated_tasks);
    }
    return curr_donated_tasks;
}


void Coordinator::balanceLoad() {
    int total_tasks = 0;
    for (auto &scheduler : schedulers) {
        int current_task_count = scheduler->current_task_count;
        total_tasks += current_task_count;
    }
    int average_tasks_floor = total_tasks / schedulers.size();
    int average_tasks_ceil = std::ceil(((total_tasks * 1.0) / schedulers.size()));

    std::deque<std::pair<Scheduler*, int>> doner_schedulers;
    for (auto &scheduler : schedulers) {
        int current_task_count = scheduler->current_task_count;
        if (current_task_count > average_tasks_ceil) {
            doner_schedulers.push_back({scheduler, current_task_count});
        }
    }

    logger.trace("Periodic balancing total_tasks: %d, average_tasks_ceil: %d, average_tasks_floor: %d, doner_schedulers_size: %d", total_tasks, average_tasks_ceil, average_tasks_floor, doner_schedulers.size());
    int total_donated_tasks = 0;
    for (auto &scheduler : schedulers) {
        int current_task_count = scheduler->current_task_count;
        if (current_task_count < average_tasks_floor) {
            int max_acceptable_donations = average_tasks_floor - current_task_count;
            while (!doner_schedulers.empty() && max_acceptable_donations > 0) {
                Scheduler *doner_scheduler = doner_schedulers.back().first;
                int doner_current_task_count = doner_schedulers.back().second;
                if (doner_current_task_count < average_tasks_ceil || total_donated_tasks == MAX_TASKS_TO_STEAL) {
                    doner_schedulers.pop_back();
                    total_donated_tasks = 0;
                    continue;
                }
                int curr_donated_tasks = std::min(max_acceptable_donations, doner_current_task_count - average_tasks_ceil);
                curr_donated_tasks = std::min(curr_donated_tasks, MAX_TASKS_TO_STEAL);
                if (curr_donated_tasks > 0) {
                    max_acceptable_donations = max_acceptable_donations - curr_donated_tasks;
                    doner_scheduler->submitToOwnerSubmissionQueue(curr_donated_tasks, scheduler);
                    doner_schedulers.back().second -= curr_donated_tasks;
                    total_donated_tasks += curr_donated_tasks;
                }
            }
        }
    }
}


void Coordinator::submit(Task* task) {
    schedulers[next_scheduler_id]->submit(task);
    #ifndef ENABLE_LOAD_BALANCE_METRICS
    next_scheduler_id = (next_scheduler_id + 1) % schedulers.size();
    #endif
}

void Coordinator::start() {
    #ifdef ENABLE_PINNING
    #ifdef __linux__
    logger.info("Thread pinning enabled");
    #endif
    #endif

    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (auto &scheduler : schedulers) {
        scheduler->setCoordinator(this);
        threads.emplace_back([&scheduler, core_to_run] {
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
                logger.error("Scheduler %d caught runtime error: %s", scheduler->id, e.what());
            } catch (const std::exception& e) {
                logger.error("Scheduler %d caught exception: %s", scheduler->id, e.what());
            } catch (...) {
                logger.error("Scheduler %d caught unknown exception", scheduler->id);
            }
        });
        core_to_run = (core_to_run + 2) % max_cores;
    }
    threads.emplace_back([this] {
        try {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distribution(500, 1500);
            while(true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(distribution(gen)));
                balanceLoad();
            }
        } catch (const std::runtime_error& e) {
            logger.error("Load balancing deamon caught runtime error: %s", e.what());
        } catch (const std::exception& e) {
            logger.error("Load balancing deamon caught exception: %s", e.what());
        } catch (...) {
            logger.error("Load balancing deamon caught unknown exception");
        }
    });

    for (std::thread& thread : threads) {
        thread.join();
    }
}

void Coordinator::stop() {
    for (auto &scheduler : schedulers) {
        scheduler->stop();
    }
}
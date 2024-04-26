#include <thread>
#include "coordinator.h"

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

void Coordinator::submit(Task* task) {
    schedulers[next_scheduler_id]->submit(task);
    next_scheduler_id = (next_scheduler_id + 1) % schedulers.size();
}

void Coordinator::start() {
    #ifdef __linux__
    std::cout << "Thread pinning enabled" << std::endl;
    #endif

    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (auto &scheduler : schedulers) {
        threads.emplace_back([&scheduler, core_to_run] {
            try {
                // Set CPU affinity
                #ifdef __linux__
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);       
                CPU_SET(core_to_run, &cpuset);
                if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                    throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
                }
                #endif
                
                scheduler->start(); 
            } catch (const std::runtime_error& e) {
                std::cout << "Caught runtime error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Caught exception: " << e.what() << std::endl;
            } catch (...) {
                std::cout << "Caught unknown exception" << std::endl;
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
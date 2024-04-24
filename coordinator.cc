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
        Scheduler *scheduler = schedulers.back();
        schedulers.pop_back();
        delete scheduler;
    }
}

void Coordinator::submit(Task* task) {
    schedulers[next_scheduler_id]->submit(task);
    next_scheduler_id = (next_scheduler_id + 1) % schedulers.size();
}

void Coordinator::start() {
    std::vector<std::thread> threads;
    for (auto &scheduler : schedulers) {
        threads.emplace_back([&scheduler] {
            try {
                scheduler->start(); 
            } catch (const std::runtime_error& e) {
                std::cout << "Caught runtime error: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Caught exception: " << e.what() << std::endl;
            } catch (...) {
                std::cout << "Caught unknown exception" << std::endl;
            }
        });
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
#include <thread>
#include "coordinator.h"

Coordinator::Coordinator() {
    int max_threads = std::thread::hardware_concurrency();
    this->file_scheduler = new FileScheduler();
    for (int i = 0; i < max_threads - 2; ++i) {
        Scheduler* scheduler = new Scheduler(i);
        scheduler->setFileScheduler(file_scheduler);
        this->schedulers.push_back(scheduler);
    }
    file_scheduler->setSchedulers(&this->schedulers);
}

Coordinator::~Coordinator() {
    if (file_scheduler != nullptr) {
        delete file_scheduler;
    }
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
        threads.emplace_back([&scheduler] { scheduler->start(); });
    }
    threads.emplace_back([this] { file_scheduler->start_consumer(); });
    threads.emplace_back([this] { file_scheduler->start_producer(); });
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void Coordinator::stop() {
    if (file_scheduler != nullptr) {
        file_scheduler->stop();
    }
    for (auto &scheduler : schedulers) {
        scheduler->stop();
    }
}
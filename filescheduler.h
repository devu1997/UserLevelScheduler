#pragma once

#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <unordered_map>
#include <liburing.h>
#include "filetasks.h"


#define QUEUE_DEPTH 1
#define BLOCK_SZ    1024

class Scheduler;

struct FileInfo {
    int task_id;
    off_t file_size;
    struct iovec iovecs[];
};

class FileScheduler {
private:
    bool stop_flag;
    std::vector<Scheduler*> *schedulers;
    struct io_uring ring;
    std::mutex producer_mtx;
    std::mutex consumer_mtx;
    std::condition_variable producer_cv;
    std::condition_variable consumer_cv;
    std::deque<FileReadCompleteTask*> queued_requests;
    std::unordered_map<int, FileReadCompleteTask*> in_process_requests;

public:
    FileScheduler();
    ~FileScheduler();

    void submit(FileReadCompleteTask *task);
    void start_producer();
    void start_consumer();
    void stop();
    void setSchedulers(std::vector<Scheduler*> *schedulers);

private:
    off_t get_file_size(int fd);
};

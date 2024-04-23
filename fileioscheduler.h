#pragma once

#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <mutex>
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
    Scheduler *scheduler;
    struct io_uring ring;
    std::mutex mtx;
    std::deque<FileReadTask*> queued_requests;
    std::unordered_map<int, FileReadTask*> in_process_requests;

public:
    FileScheduler();
    ~FileScheduler();

    void submit(FileReadTask *task);
    void start_producer();
    void start_consumer();
    void setScheduler(Scheduler *scheduler);

private:
    off_t get_file_size(int fd);
};

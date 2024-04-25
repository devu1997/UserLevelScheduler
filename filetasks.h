#pragma once

#include <iostream>
#include <chrono>
#include <functional>
#include <string.h>
#include "task.h"

/* File close */

struct FileCloseTaskInput {
    int file_fd;
};

class FileCloseTask : public Task {
public:
    using Task::Task;

    void* process() override;
    Task* fork() override;
};

/* File read */

struct FileReadTaskInput {
    int file_fd;
};

struct FileReadTaskOutput : public FileCloseTaskInput {
    std::string data;
    FileReadTaskOutput(int file_fd, std::string data) : FileCloseTaskInput{file_fd} {
        this->data = data;
    }
};

class FileReadTask : public Task {
public:
    using Task::Task;

    void* process() override;
    Task* fork() override;
};

struct AsyncFileReadTaskInput {
    int file_fd;
    off_t file_size;
    int blocks;
    struct iovec iovecs[];
};

class AsyncFileReadTask : public Task {
public:
    using Task::Task;
    
    std::chrono::steady_clock::time_point start_time;

    void* process() override;
    Task* fork() override;

    void setStartTime();
};

/* File open */

struct FileOpenTaskInput {
    const char* file_path;
    int mode;
};

struct FileOpenTaskOutput : public FileReadTaskInput {
    FileOpenTaskOutput(int file_fd) : FileReadTaskInput{file_fd} {}
};

class FileOpenTask : public Task {
public:
    using Task::Task;

    void* process() override;
    Task* fork() override;
};
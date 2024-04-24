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
    FileCloseTask(std::function<void*()> func, bool forward_result = false);
    FileCloseTask(bool forward_result = true);

    void* process() override;
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
    FileReadTask(std::function<void*()> func, bool forward_result = true);
    FileReadTask(bool forward_result = true);

    void* process() override;
};

struct FileReadCompleteTaskInput {
    int file_fd;
    off_t file_size;
    int blocks;
    struct iovec iovecs[];
};

class FileReadCompleteTask : public Task {
public:
    std::chrono::_V2::steady_clock::time_point start_time;

    FileReadCompleteTask(std::function<void*()> func, bool forward_result = true);

    void* process() override;
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
    FileOpenTask(std::function<void*()> func, bool forward_result = true);
    FileOpenTask(bool forward_result = true);

    void* process() override;
};
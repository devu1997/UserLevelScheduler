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
private:
    std::function<void*(void*)> func;

public:
    FileCloseTask();
    FileCloseTask(std::function<void*(void*)> func);

    void* process() override;
    Task* fork() override;
};

/* File read */

struct FileReadTaskInput {
    int file_fd;
};

struct FileReadTaskOutput : public FileCloseTaskInput {
    struct iovec* iovecs;
    FileReadTaskOutput(int file_fd, struct iovec* iovecs) : FileCloseTaskInput{file_fd} {
        this->iovecs = iovecs;
    }
};

class FileReadTask : public Task {
private:
    std::function<void*(void*, void*)> func;

public:
    FileReadTask();
    FileReadTask(std::function<void*(void*, void*)> func);

    void* process() override;
    Task* fork() override;
};

/* File write */

struct FileWriteTaskInput {
    int file_fd;
    off_t file_size;
    int blocks;
    struct iovec iovecs[];
};

struct FileWriteTaskOutput : public FileCloseTaskInput {
    struct iovec* iovecs;
    FileWriteTaskOutput(int file_fd, struct iovec* iovecs) : FileCloseTaskInput{file_fd} {
        this->iovecs = iovecs;
    }
};

class FileWriteTask : public Task {
private:
    std::function<void*(void*, void*)> func;

public:
    FileWriteTask();
    FileWriteTask(std::function<void*(void*, void*)> func);

    void* process() override;
    Task* fork() override;
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
private:
    std::function<void*(void*, void*)> func;

public:
    FileOpenTask();
    FileOpenTask(std::function<void*(void*, void*)> func);

    void* process() override;
    Task* fork() override;
};

/* Async tasks */

struct AsyncFileTaskInput {
    int file_fd;
    off_t file_size;
    int blocks;
    struct iovec iovecs[];
};

class AsyncFileTask : public Task {
public:
    using Task::Task;
    
    std::chrono::steady_clock::time_point start_time;

    void setStartTime();
};

/* Async read task */

class AsyncFileReadTask : public AsyncFileTask {
private:
    void* read_input;
    std::function<void*(void*, void*)> func;

public:
    AsyncFileReadTask(void* read_input, std::function<void*(void*, void*)> func);
    
    void* process() override;
    Task* fork() override;
};

/* Async write task */

class AsyncFileWriteTask : public AsyncFileTask {
private:
    void* write_input;
    std::function<void*(void*, void*)> func;

public:
    AsyncFileWriteTask(void* write_input, std::function<void*(void*, void*)> func);

    void* process() override;
    Task* fork() override;
};
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
    std::string file_path;
    int oflag;
    mode_t mode;
};

struct FileOpenTaskOutput {
    int file_fd;
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

struct FileTaskInput : public FileCloseTaskInput {
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
    std::function<void*(void*, void*)> func;

public:
    AsyncFileReadTask(std::function<void*(void*, void*)> func);
    
    void* process() override;
    Task* fork() override;
};

/* Async write task */

class AsyncFileWriteTask : public AsyncFileTask {
private:
    std::function<void*(void*, void*)> func;

public:
    AsyncFileWriteTask(std::function<void*(void*, void*)> func);

    void* process() override;
    Task* fork() override;
};
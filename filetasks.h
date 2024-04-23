#pragma once

#include <iostream>
#include <string.h>
#include "task.h"


struct FileCloseTaskInput {
    int file_fd;
};

class FileCloseTask : public Task {
public:
    FileCloseTask(bool forward_result = false);

    void* process() override;
};

struct FileReadTaskInput {
    int file_fd;
};

class FileReadTask : public Task {
public:
    FileReadTask(bool forward_result = true);

    void* process() override;
};

struct FileReadCompleteTaskInput {
    int file_fd;
    std::string data;
};

struct FileReadCompleteTaskOutput : public FileCloseTaskInput {
    std::string data;
    FileReadCompleteTaskOutput(int file_fd, std::string data) : FileCloseTaskInput{file_fd} {
        this->data = data;
    }
};

class FileReadCompleteTask : public Task {
public:
    int scheduler_id;

    FileReadCompleteTask(int scheduler_id, FileReadTask* file_task);

    void* process() override;
    void setData(std::string &data);
};

struct FileOpenTaskInput {
    const char* file_path;
    int mode;
};

struct FileOpenTaskOutput : public FileReadTaskInput {
    FileOpenTaskOutput(int file_fd) : FileReadTaskInput{file_fd} {}
};

class FileOpenTask : public Task {
public:
    FileOpenTask(bool forward_result = true);

    void* process() override;
};
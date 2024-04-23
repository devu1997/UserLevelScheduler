#pragma once

#include <iostream>
#include <string.h>
#include "task.h"


struct FileOpenTaskInput {
    const char* file_path;
    int mode;
};

struct FileOpenTaskOutput {
    int file_fd;
};

class FileOpenTask : public Task {
public:
    FileOpenTask(bool forward_result = true);

    void* process() override;
};

struct FileReadTaskInput {
    int file_fd;
};

struct FileReadTaskOutput {
    int file_fd;
    std::string data;
};

class FileReadTask : public Task {
public:
    FileReadTask(bool forward_result = true);

    void* process() override;
    FileReadTaskOutput* process_completion(std::string data);
};

struct FileCloseTaskInput {
    int file_fd;
};

class FileCloseTask : public Task {
public:
    FileCloseTask(bool forward_result = false);

    void* process() override;
};

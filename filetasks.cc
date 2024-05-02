
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>
#include <linux/fs.h>
#include "filetasks.h"

#define BLOCK_SZ    1024

off_t get_file_size(int fd) {
    struct stat st;
    if(fstat(fd, &st) < 0) {
        throw std::runtime_error("Error: fstat failed");
    }
    if (S_ISBLK(st.st_mode)) {
        unsigned long long bytes;
        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
            throw std::runtime_error("Error: ioctl failed");
        }
        return bytes;
    } else if (S_ISREG(st.st_mode)) {
        return st.st_size;
    }
    return -1;
}

/* File open */ 

FileOpenTask::FileOpenTask() {
    this->func = [this](void* in, void* out) -> void* {
        return out;
    };
}

FileOpenTask::FileOpenTask(std::function<void*(void*, void*)> func) {
    this->func = func;
}

void* FileOpenTask::process() {
    FileOpenTaskInput* fo_input = static_cast<FileOpenTaskInput*>(input);
    int file_fd = open(fo_input->file_path.c_str(), fo_input->oflag, fo_input->mode);
    if (file_fd < 0) {
        throw std::runtime_error(std::string("Error: Unable to open file: ") + fo_input->file_path + std::strerror(errno));
    }
    FileOpenTaskOutput* fo_output = new FileOpenTaskOutput({file_fd});
    logger.trace("Opened file: %d", file_fd);
    return func(fo_input, fo_output);
}

Task* FileOpenTask::fork() {
    FileOpenTask* task = new FileOpenTask(this->func);
    this->copy(task);
    return task;
}

/* File read */ 

FileReadTask::FileReadTask() {
    this->func = [this](void* in, void* out) -> void* {
        return out;
    };
}

FileReadTask::FileReadTask(std::function<void*(void*, void*)> func) {
    this->func = func;
}

void* FileReadTask::process() {
    AsyncFileReadTask* async_task = new AsyncFileReadTask(this->func);
    async_task->setForwardResult(this->forward_result);
    async_task->setNextTasks(this->next_tasks);
    async_task->setExecutionMode(TaskExecutionMode::ASYNC_FILE);
    this->next_tasks = {async_task};
    this->forward_result = true;
    logger.trace("Start read file");
    return input;
}

Task* FileReadTask::fork() {
    FileReadTask* task = new FileReadTask(this->func);
    this->copy(task);
    return task;
}

/* File write */ 

FileWriteTask::FileWriteTask() {
    this->func = [this](void* in, void* out) -> void* {
        return out;
    };
}

FileWriteTask::FileWriteTask(std::function<void*(void*, void*)> func) {
    this->func = func;
}

void* FileWriteTask::process() {
    AsyncFileWriteTask* async_task = new AsyncFileWriteTask(this->func);
    async_task->setForwardResult(this->forward_result);
    async_task->setNextTasks(this->next_tasks);
    async_task->setExecutionMode(TaskExecutionMode::ASYNC_FILE);
    this->next_tasks = {async_task};
    this->forward_result = true;
    logger.trace("Start writing file");
    return input;
}

Task* FileWriteTask::fork() {
    FileWriteTask* task = new FileWriteTask(this->func);
    this->copy(task);
    return task;
}

/* File close */

FileCloseTask::FileCloseTask() {
    this->func = [this](void* in) -> void* {
        return nullptr;
    };
}

FileCloseTask::FileCloseTask(std::function<void*(void*)> func) {
    this->func = func;
}

void* FileCloseTask::process() {
    FileCloseTaskInput* fc_input = static_cast<FileCloseTaskInput*>(input);
    int ret = close(fc_input->file_fd);
    if (ret == -1) {
        throw std::runtime_error("Error: Unable to close file");
    }
    logger.trace("Closed file: %d", fc_input->file_fd);
    return func(fc_input);
}

Task* FileCloseTask::fork() {
    FileCloseTask* task = new FileCloseTask(this->func);
    this->copy(task);
    return task;
}

/* Async tasks */ 

void AsyncFileTask::setStartTime() {
    this->start_time = std::chrono::steady_clock::now();
}

/* Async read task */ 

AsyncFileReadTask::AsyncFileReadTask(std::function<void*(void*, void*)> func) {
    this->func = func;
}

void* AsyncFileReadTask::process() {
    FileTaskInput* ft_input = static_cast<FileTaskInput*>(input);
    logger.trace("Read file");
    return func(input, ft_input);
}

Task* AsyncFileReadTask::fork() {
    AsyncFileReadTask* task = new AsyncFileReadTask(this->func);
    this->copy(task);
    return task;
}

/* Async write task */ 

AsyncFileWriteTask::AsyncFileWriteTask(std::function<void*(void*, void*)> func) {
    this->func = func;
}

void* AsyncFileWriteTask::process() {
    FileTaskInput* ft_input = static_cast<FileTaskInput*>(input);
    logger.trace("Write file");
    return func(input, ft_input);
}

Task* AsyncFileWriteTask::fork() {
    AsyncFileWriteTask* task = new AsyncFileWriteTask(this->func);
    this->copy(task);
    return task;
}

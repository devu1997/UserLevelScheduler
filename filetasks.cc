
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/ioctl.h>
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
    std::cout<<"Open file"<<std::endl;
    int file_fd = open(fo_input->file_path, fo_input->mode);
    if (file_fd < 0) {
        throw std::runtime_error("Error: Unable to open file");
    }
    FileOpenTaskOutput *fo_output = new FileOpenTaskOutput({file_fd});
    return func(fo_input, fo_output);
}

Task* FileOpenTask::fork() {
    FileOpenTask *task = new FileOpenTask(this->func);
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
    FileReadTaskInput* fr_input = static_cast<FileReadTaskInput*>(input);
    int file_fd = fr_input->file_fd;
    off_t file_size = get_file_size(file_fd);
    off_t bytes_remaining = file_size;
    off_t offset = 0;
    int current_block = 0;
    int blocks = (int) file_size / BLOCK_SZ;
    if (file_size % BLOCK_SZ) blocks++;
    AsyncFileTaskInput *frc_input = (AsyncFileTaskInput*) malloc(sizeof(*frc_input) + (sizeof(struct iovec) * blocks));
    char *buff = (char*)malloc(file_size);
    if (!buff) {
        throw std::runtime_error("Error: Unable to allocate memory for file read");
    }
    while (bytes_remaining) {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ) {
            bytes_to_read = BLOCK_SZ;
        }
        frc_input->iovecs[current_block].iov_len = bytes_to_read;
        void *buf;
        if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)) {
            throw std::runtime_error("Error: Posix memalign failed");
        }
        frc_input->iovecs[current_block].iov_base = buf;
        current_block++;
        bytes_remaining -= bytes_to_read;
    }
    frc_input->file_fd = file_fd;
    frc_input->file_size = file_size;
    frc_input->blocks = blocks;
    AsyncFileReadTask* async_task = new AsyncFileReadTask(this->input, this->func);
    async_task->setForwardResult(this->forward_result);
    async_task->setNextTasks(this->next_tasks);
    async_task->setExecutionMode(TaskExecutionMode::ASYNC_FILE);
    this->next_tasks = {async_task};
    this->forward_result = true;
    std::cout<<"Start read file of size: "<<file_size<<" block: "<<blocks<<std::endl;
    return frc_input;
}

Task* FileReadTask::fork() {
    FileReadTask *task = new FileReadTask(this->func);
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
    std::cout<<"Closed file"<<std::endl;
    return func(fc_input);
}

Task* FileCloseTask::fork() {
    FileCloseTask *task = new FileCloseTask(this->func);
    this->copy(task);
    return task;
}

/* Async tasks */

void AsyncFileTask::setStartTime() {
    this->start_time = std::chrono::steady_clock::now();
}

/* Async read task */

AsyncFileReadTask::AsyncFileReadTask(void* read_input, std::function<void*(void*, void*)> func) {
    this->func = func;
    this->read_input = read_input;
}

void* AsyncFileReadTask::process() {
    AsyncFileTaskInput* frc_input = static_cast<AsyncFileTaskInput*>(input);
    int blocks = (int) frc_input->file_size / BLOCK_SZ;
    if (frc_input->file_size % BLOCK_SZ) blocks++;
    std::string data = "";
    for (int i = 0; i < blocks; i ++)
        data += std::string((char*)frc_input->iovecs[i].iov_base, frc_input->iovecs[i].iov_len);
    std::cout<<data<<std::endl;
    FileReadTaskOutput *fr_output = new FileReadTaskOutput({frc_input->file_fd, frc_input->iovecs});
    std::cout<<"Read file "<<this->next_tasks.size()<<std::endl;
    return func(read_input, fr_output);
}

Task* AsyncFileReadTask::fork() {
    AsyncFileReadTask *task = new AsyncFileReadTask(this->read_input, this->func);
    this->copy(task);
    return task;
}

/* Async write task */

AsyncFileWriteTask::AsyncFileWriteTask(void* write_input, std::function<void*(void*, void*)> func) {
    this->func = func;
    this->write_input = write_input;
}

void* AsyncFileWriteTask::process() {
    AsyncFileTaskInput* frc_input = static_cast<AsyncFileTaskInput*>(input);
    int blocks = (int) frc_input->file_size / BLOCK_SZ;
    if (frc_input->file_size % BLOCK_SZ) blocks++;
    std::string data = "";
    for (int i = 0; i < blocks; i ++)
        data += std::string((char*)frc_input->iovecs[i].iov_base, frc_input->iovecs[i].iov_len);
    // std::cout<<data<<std::endl;
    FileWriteTaskOutput *fr_output = new FileWriteTaskOutput({frc_input->file_fd, frc_input->iovecs});
    std::cout<<"Write file "<<this->next_tasks.size()<<std::endl;
    return func(write_input, fr_output);
}

Task* AsyncFileWriteTask::fork() {
    AsyncFileWriteTask *task = new AsyncFileWriteTask(this->write_input, this->func);
    this->copy(task);
    return task;
}

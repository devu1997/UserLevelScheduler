
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

FileOpenTask::FileOpenTask(std::function<void*()> func, bool forward_result) : Task(func, forward_result, TaskExecutionMode::SYNC) {}

FileOpenTask::FileOpenTask(bool forward_result) : Task(forward_result, TaskExecutionMode::SYNC) {}

void* FileOpenTask::process() {
    FileOpenTaskInput* fo_input = static_cast<FileOpenTaskInput*>(input);
    int file_fd = open(fo_input->file_path, fo_input->mode);
    if (file_fd < 0) {
        throw std::runtime_error("Error: Unable to open file");
    }
    FileOpenTaskOutput *fo_output = new FileOpenTaskOutput({file_fd});
    return fo_output;
}

/* File read */

FileReadTask::FileReadTask(std::function<void*()> func, bool forward_result) : Task(func, forward_result, TaskExecutionMode::SYNC) {}

FileReadTask::FileReadTask(bool forward_result) : Task(forward_result, TaskExecutionMode::SYNC) {}

void* FileReadTask::process() {
    FileReadTaskInput* fr_input = static_cast<FileReadTaskInput*>(input);
    int file_fd = fr_input->file_fd;
    off_t file_size = get_file_size(file_fd);
    off_t bytes_remaining = file_size;
    off_t offset = 0;
    int current_block = 0;
    int blocks = (int) file_size / BLOCK_SZ;
    if (file_size % BLOCK_SZ) blocks++;
    FileReadCompleteTaskInput *frc_input = (FileReadCompleteTaskInput*) malloc(sizeof(*frc_input) + (sizeof(struct iovec) * blocks));
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
    FileReadCompleteTask* file_complete_task = new FileReadCompleteTask(this->func, this->forward_result);
    file_complete_task->setNextTasks(this->next_tasks);
    this->next_tasks = {file_complete_task};
    this->forward_result = true;
    return frc_input;
}

FileReadCompleteTask::FileReadCompleteTask(std::function<void*()> func, bool forward_result) : Task(func, forward_result, TaskExecutionMode::ASYNC_FILE) {}

void* FileReadCompleteTask::process() {
    FileReadCompleteTaskInput* frc_input = static_cast<FileReadCompleteTaskInput*>(input);
    int blocks = (int) frc_input->file_size / BLOCK_SZ;
    if (frc_input->file_size % BLOCK_SZ) blocks++;
    std::string data = "";
    for (int i = 0; i < blocks; i ++)
        data += std::string((char*)frc_input->iovecs[i].iov_base, frc_input->iovecs[i].iov_len);
    // std::cout<<data<<std::endl;
    FileReadTaskOutput *fr_output = new FileReadTaskOutput({frc_input->file_fd, data});
    return fr_output;
}

void FileReadCompleteTask::setStartTime() {
    this->start_time = std::chrono::steady_clock::now();
}

/* File close */

FileCloseTask::FileCloseTask(std::function<void*()> func, bool forward_result) : Task(func, forward_result, TaskExecutionMode::SYNC) {}

FileCloseTask::FileCloseTask(bool forward_result) : Task(forward_result, TaskExecutionMode::SYNC) {}

void* FileCloseTask::process() {
    FileCloseTaskInput* fc_input = static_cast<FileCloseTaskInput*>(input);
    int ret = close(fc_input->file_fd);
    if (ret == -1) {
        throw std::runtime_error("Error: Unable to close file");
    }
    return nullptr;
}


#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include "filetasks.h"

FileOpenTask::FileOpenTask(bool forward_result) : Task(forward_result, TaskExecutionMode::SYNC) {}

void* FileOpenTask::process() {
    std::cout<<"OPENING"<<std::endl;
    FileOpenTaskInput* fo_input = static_cast<FileOpenTaskInput*>(input);
    int file_fd = open(fo_input->file_path, fo_input->mode);
    if (file_fd < 0) {
        throw std::runtime_error("Error: Unable to open file");
    }
    FileOpenTaskOutput *fo_output = new FileOpenTaskOutput({file_fd});
    return fo_output;
}

FileReadTask::FileReadTask(bool forward_result) : Task(forward_result, TaskExecutionMode::ASYNC_FILE) {}

void* FileReadTask::process() {
    return nullptr;
}

FileReadTaskOutput* FileReadTask::process_completion(std::string data) {
    std::cout<<"READ COMPLETE "<<data<<std::endl;
    FileReadTaskInput* fr_input = static_cast<FileReadTaskInput*>(input);
    FileReadTaskOutput *fr_output = new FileReadTaskOutput({fr_input->file_fd, data});
    return fr_output;
}

FileCloseTask::FileCloseTask(bool forward_result) : Task(forward_result, TaskExecutionMode::SYNC) {}

void* FileCloseTask::process() {
    std::cout<<"CLOSING"<<std::endl;
    FileCloseTaskInput* fc_input = static_cast<FileCloseTaskInput*>(input);
    int ret = close(fc_input->file_fd);
    if (ret == -1) {
        throw std::runtime_error("Error: Unable to close file");
    }
    return nullptr;
}

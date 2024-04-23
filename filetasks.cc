
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include "filetasks.h"

FileOpenTask::FileOpenTask(bool forward_result) : Task(forward_result) {}

void* FileOpenTask::process() {
    FileOpenTaskInput* fo_input = static_cast<FileOpenTaskInput*>(input);
    int file_fd = open(fo_input->file_path, fo_input->mode);
    if (file_fd < 0) {
        throw std::runtime_error("Error: Unable to open file");
    }
    FileOpenTaskOutput *fo_output = new FileOpenTaskOutput({file_fd});
    return fo_output;
}

FileReadTask::FileReadTask(bool forward_result) : Task(forward_result) {}

void* FileReadTask::process() {
    // ioscheduler.submit_for_read(this);
    // Do not process the next tasks
    return process_completion("wohoooo");
}

FileReadTaskOutput* FileReadTask::process_completion(std::string data) {
    FileReadTaskInput* fr_input = static_cast<FileReadTaskInput*>(input);
    FileReadTaskOutput *fr_output = new FileReadTaskOutput({fr_input->file_fd, data});
    std::cout<<"Reched processing 2.3";
    return fr_output;
}

FileCloseTask::FileCloseTask(bool forward_result) : Task(forward_result) {}

void* FileCloseTask::process() {
    std::cout<<"CLOSING 3";
    FileCloseTaskInput* fc_input = static_cast<FileCloseTaskInput*>(input);
    int ret = close(fc_input->file_fd);
    if (ret == -1) {
        throw std::runtime_error("Error: Unable to close file");
    }
    return nullptr;
}

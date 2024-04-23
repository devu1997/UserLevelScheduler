
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include "filetasks.h"

FileOpenTask::FileOpenTask(bool forward_result) : Task(forward_result, TaskExecutionMode::SYNC) {}

void* FileOpenTask::process() {
    FileOpenTaskInput* fo_input = static_cast<FileOpenTaskInput*>(input);
    int file_fd = open(fo_input->file_path, fo_input->mode);
    if (file_fd < 0) {
        throw std::runtime_error("Error: Unable to open file");
    }
    FileOpenTaskOutput *fo_output = new FileOpenTaskOutput({file_fd});
    std::cout<<"OPENING"<<std::endl;;
    return fo_output;
}

FileReadTask::FileReadTask(bool forward_result) : Task(forward_result, TaskExecutionMode::ASYNC_FILE) {}

void* FileReadTask::process() {
    return nullptr;
}

FileReadCompleteTask::FileReadCompleteTask(int scheduler_id, FileReadTask* file_task) : Task(file_task->forward_result, TaskExecutionMode::SYNC) {
    this->scheduler_id = scheduler_id;
    this->next_tasks = file_task->next_tasks;
    FileReadTaskInput* fr_input = static_cast<FileReadTaskInput*>(file_task->input);
    this->input = new FileReadCompleteTaskInput({fr_input->file_fd, ""});
}

void* FileReadCompleteTask::process() {
    FileReadCompleteTaskInput* frc_input = static_cast<FileReadCompleteTaskInput*>(input);
    std::cout<<"READ COMPLETE "<<frc_input->data<<std::endl;
    FileReadCompleteTaskOutput *fr_output = new FileReadCompleteTaskOutput({frc_input->file_fd, frc_input->data});
    return fr_output;
}

void FileReadCompleteTask::setData(std::string &data) {
    FileReadCompleteTaskInput* frc_input = static_cast<FileReadCompleteTaskInput*>(input);
    frc_input->data = data;
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

#include <stdexcept>
#include "filescheduler.h"


#define QUEUE_DEPTH 32

FileScheduler::FileScheduler() {
    int ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret < 0) {
        throw std::runtime_error("Error: Unable to initialize IO uring");
    }
}

FileScheduler::~FileScheduler() {
    io_uring_queue_exit(&ring);
}

void FileScheduler::setScheduler(Scheduler* scheduler) {
    this->scheduler = scheduler;
}

void FileScheduler::submit(FileReadCompleteTask *task) {
    FileReadCompleteTaskInput* fr_input = static_cast<FileReadCompleteTaskInput*>(task->input);
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_readv(sqe, fr_input->file_fd, fr_input->iovecs, fr_input->blocks, 0);
    io_uring_sqe_set_data(sqe, task);
    io_uring_submit(&ring);
}

void FileScheduler::process_completed() {
    struct io_uring_cqe *cqe;
    unsigned head;
    int processed{0};
    io_uring_for_each_cqe(&ring, head, cqe) {
        if (cqe->res < 0) {
            throw std::runtime_error("Error: Async readv failed");
        }
        FileReadCompleteTask *task = (FileReadCompleteTask*)io_uring_cqe_get_data(cqe);
        task->exec_mode = TaskExecutionMode::SYNC;
        FileReadCompleteTaskInput* fr_input = static_cast<FileReadCompleteTaskInput*>(task->input);
        processed += fr_input->blocks;
        scheduler->submit(task);
    }
    io_uring_cq_advance(&ring, processed);
}


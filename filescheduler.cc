#include <stdexcept>
#include "filescheduler.h"


#define QUEUE_DEPTH 64

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

void FileScheduler::submit(AsyncFileReadTask *task) {
    AsyncFileTaskInput* fr_input = static_cast<AsyncFileTaskInput*>(task->input);
    task->setStartTime();
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    io_uring_prep_readv(sqe, fr_input->file_fd, fr_input->iovecs, fr_input->blocks, 0);
    io_uring_sqe_set_data(sqe, task);
    io_uring_submit(&ring);
    pending_requests++;
}

// void FileScheduler::submit(AsyncFileWriteTask *task) {
//     AsyncFileWriteTaskInput* fr_input = static_cast<AsyncFileWriteTaskInput*>(task->input);
//     task->setStartTime();
//     struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
//     io_uring_prep_writev(sqe, fr_input->file_fd, fr_input->iovecs, fr_input->blocks, 0);
//     io_uring_sqe_set_data(sqe, task);
//     io_uring_submit(&ring);
//     pending_requests++;
// }

void FileScheduler::process_completed() {
    if (pending_requests == 0) return;
    struct io_uring_cqe *cqe;
    unsigned head;
    int processed{0};
    io_uring_for_each_cqe(&ring, head, cqe) {
        if (cqe->res < 0) {
            throw std::runtime_error("Error: Async readv failed");
        }
        AsyncFileTask *task = (AsyncFileTask*)io_uring_cqe_get_data(cqe);
        AsyncFileTaskInput* fr_input = static_cast<AsyncFileTaskInput*>(task->input);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - task->start_time);
        task->history.addEvent({EventType::IO, duration});
        task->setExecutionMode(TaskExecutionMode::SYNC);
        scheduler->submit(task);
        processed += fr_input->blocks;
        pending_requests--;
    }
    io_uring_cq_advance(&ring, processed);
}


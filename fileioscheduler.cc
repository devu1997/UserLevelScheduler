#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdexcept>
#include "fileioscheduler.h"


FileScheduler::FileScheduler() {
    int s = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (s < 0) {
        throw std::runtime_error("Error: Unable to initialize IO uring");
    }
}

FileScheduler::~FileScheduler() {
    io_uring_queue_exit(&ring);
}


void FileScheduler::setScheduler(Scheduler *scheduler) {
    this->scheduler = scheduler;
}

void FileScheduler::submit(FileReadTask *task) {
    std::lock_guard<std::mutex> lock(mtx);
    queued_requests.push_back(task);
}

void FileScheduler::start_producer() {
    while (true) {
        if (!queued_requests.empty()) {
            FileReadTask *task;
            {
                std::lock_guard<std::mutex> lock(mtx);
                task = queued_requests.front();
                queued_requests.pop_front();
            }
            FileReadTaskInput* fr_input = static_cast<FileReadTaskInput*>(task->input);
            int file_fd = fr_input->file_fd;
            int task_id = task->id;
            off_t file_size = get_file_size(file_fd);
            off_t bytes_remaining = file_size;
            off_t offset = 0;
            int current_block = 0;
            int blocks = (int) file_size / BLOCK_SZ;
            if (file_size % BLOCK_SZ) blocks++;
            FileInfo *fi = (FileInfo*) malloc(sizeof(*fi) + (sizeof(struct iovec) * blocks));
            char *buff = (char*)malloc(file_size);
            if (!buff) {
                throw std::runtime_error("Error: Unable to allocate memory for file read");
            }
            while (bytes_remaining) {
                off_t bytes_to_read = bytes_remaining;
                if (bytes_to_read > BLOCK_SZ) {
                    bytes_to_read = BLOCK_SZ;
                }
                fi->iovecs[current_block].iov_len = bytes_to_read;
                void *buf;
                if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)) {
                    throw std::runtime_error("Error: Posix memalign failed");
                }
                fi->iovecs[current_block].iov_base = buf;
                current_block++;
                bytes_remaining -= bytes_to_read;
            }
            fi->file_size = file_size;
            fi->task_id = task_id;
            in_process_requests[task_id] = task;
            struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
            io_uring_prep_readv(sqe, file_fd, fi->iovecs, blocks, 0);
            io_uring_sqe_set_data(sqe, fi);
            io_uring_submit(&ring);
        }
    }
}

void FileScheduler::start_consumer() {
    while (true) {
        struct io_uring_cqe *cqe;
        unsigned head;
        int processed{0};
        io_uring_for_each_cqe(&ring, head, cqe) {
            if (cqe->res < 0) {
                throw std::runtime_error("Error: Async readv failed");
            }
            FileInfo *fi = (FileInfo*)io_uring_cqe_get_data(cqe);
            int blocks = (int) fi->file_size / BLOCK_SZ;
            if (fi->file_size % BLOCK_SZ) blocks++;
            std::string data = "";
            for (int i = 0; i < blocks; i ++)
                data += std::string((char*)fi->iovecs[i].iov_base, fi->iovecs[i].iov_len);;
            ++processed;
            FileReadTask *task = in_process_requests[fi->task_id];
            auto result = task->process_completion(data);
            if (task->next_tasks.size() > 0) {
                for (Task* next_task : task->next_tasks) {
                    if (task->forward_result) {
                        next_task->setInput(result);
                    }
                    scheduler->submit(next_task);
                }
            }
            delete in_process_requests[fi->task_id];
        }
        io_uring_cq_advance(&ring, processed);
    }
}

off_t FileScheduler::get_file_size(int fd) {
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

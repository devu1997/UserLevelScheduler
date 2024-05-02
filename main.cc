#include <iostream>
#include "logger.cc"
#include "history.cc"
#include "task.cc"
#include "cputask.cc"
#include "filetasks.cc"
#include "priority_queue.cc"
#include "calender_queue.cc"
#include "producer_consumer_queue.cc"
#include "coordinator.cc"
#include "scheduler.cc"
#include "filescheduler.cc"
#include <fcntl.h>
#include <thread>
#include <chrono>

int task_counter = 0;

std::vector<int> generatePrimes(int limit) {
    std::vector<bool> isPrime(limit + 1, true);
    std::vector<int> primes;

    for (int p = 2; p * p <= limit; ++p) {
        if (isPrime[p]) {
            for (int i = p * p; i <= limit; i += p) {
                isPrime[i] = false;
            }
        }
    }

    for (int p = 2; p <= limit; ++p) {
        if (isPrime[p]) {
            primes.push_back(p);
        }
    }

    return primes;
}

FileTaskInput* generateFileReadInput(int file_fd) {
    off_t file_size = get_file_size(file_fd);
    off_t bytes_remaining = file_size;
    off_t offset = 0;
    int current_block = 0;
    int blocks = (int) file_size / BLOCK_SZ;
    if (file_size % BLOCK_SZ) blocks++;
    FileTaskInput* ft_input = (FileTaskInput*) malloc(sizeof(*ft_input) + (sizeof(struct iovec)*  blocks));
    char* buff = (char*)malloc(file_size);
    if (!buff) {
        throw std::runtime_error("Error: Unable to allocate memory for file read");
    }
    while (bytes_remaining) {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ) {
            bytes_to_read = BLOCK_SZ;
        }
        ft_input->iovecs[current_block].iov_len = bytes_to_read;
        void* buf;
        if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ)) {
            throw std::runtime_error("Error: Posix memalign failed");
        }
        ft_input->iovecs[current_block].iov_base = buf;
        current_block++;
        bytes_remaining -= bytes_to_read;
    }
    ft_input->file_fd = file_fd;
    ft_input->file_size = file_size;
    ft_input->blocks = blocks;
    return ft_input;
}

struct FileOpenTaskInputExtention : public FileOpenTaskInput {
  FileTaskInput* ft_input;

  FileOpenTaskInputExtention(const char* file_path, int oflag, mode_t mode, FileTaskInput* ft_input) : FileOpenTaskInput({file_path, oflag, mode}) {
    this->ft_input = ft_input;
  }
};

Task* generateCpuTaskChain() {
    Task* task = new CpuTask(
      [](void*) -> void* { 
          generatePrimes(1000000);
          return nullptr;
      }
    );
    task->setForwardResult(false);
    Task* start_task = task;
    for (int i=0; i<19; i++) {
        Task* task_fork = task->fork();
        task->setNextTasks({task_fork});
        task = task_fork;
    }
    return start_task;
}

Task* generateIoTaskChain() {
    static int file_no = 0;
    std::string read_file_path = "/home/users/devika/os/UserLevelScheduler/logs/log" + std::to_string(file_no%10) + ".txt";
    std::string write_file_path = "/home/users/devika/os/UserLevelScheduler/logs/out" + std::to_string(file_no%10) + ".txt";
    Task* fro = new FileOpenTask(
        [](void* input, void* output) -> void* {
            FileOpenTaskOutput* fo_output = static_cast<FileOpenTaskOutput*>(output);
            return generateFileReadInput(fo_output->file_fd);
        }
    );
    fro->setInput(new FileOpenTaskInput({read_file_path.c_str(), O_RDONLY | O_DIRECT}));
    Task* frr = new FileReadTask();
    Task* frc = new FileCloseTask(
      [write_file_path](void* in) -> void* {
        FileTaskInput* ft_input = static_cast<FileTaskInput*>(in);
        FileOpenTaskInputExtention* out = new FileOpenTaskInputExtention(write_file_path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_DIRECT, 0644 , ft_input);
        return out;
      }
    );
    Task* fwo = new FileOpenTask(
      [](void* in, void* out) -> void* {
        FileOpenTaskInputExtention* fo_in = static_cast<FileOpenTaskInputExtention*>(in);
        FileOpenTaskOutput* fo_out = static_cast<FileOpenTaskOutput*>(out);
        fo_in->ft_input->file_fd = fo_out->file_fd;
        return fo_in->ft_input;
      }
    );
    Task* fww = new FileWriteTask();
    Task* fwc = new FileCloseTask();
    fwc->setForwardResult(false);

    Task* start_task = fro;
    for (int i=0; i<10; i++) {
      fro->setNextTasks({frr});
      frr->setNextTasks({frc});
      frc->setNextTasks({fwo});
      fwo->setNextTasks({fww});
      fww->setNextTasks({fwc});

      if (i < 9) {
        Task* fro_fork = fro->fork();
        Task* frr_fork = frr->fork();
        Task* frc_fork = frc->fork();
        Task* fwo_fork = fwo->fork();
        Task* fww_fork = fww->fork();
        Task* fwc_fork = fwc->fork();
        fwc->setNextTasks({fro_fork});
        fro = fro_fork;
        frr = frr_fork;
        frc = frc_fork;
        fwo = fwo_fork;
        fww = fww_fork;
        fwc = fwc_fork;
      }
    }
    file_no++;
    return start_task;
}

int main() {
    Coordinator coordinator;

    /* IO Intensive task chain */
    for (int i=0; i<100; i++) {
      Task *task = generateIoTaskChain();
      coordinator.submit(task);
    }
    
    /* CPU Intensive task chain */
    for (int i=0; i<100; i++) {
      Task *task = generateCpuTaskChain();
      coordinator.submit(task);
    }
    
    coordinator.start();

    return 0;
}
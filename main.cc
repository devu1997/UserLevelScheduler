#include <iostream>
#include "history.cc"
#include "task.cc"
#include "cputask.cc"
#include "filetasks.cc"
#include "priority_queue.cc"
#include "calender_queue.cc"
#include "scheduler.cc"
#include "filescheduler.cc"
#include "coordinator.cc"
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

int main() {
    Coordinator coordinator;

    Task* task1 = new CpuTask(
        [](void*) -> void* { 
            std::cout << "Running task " << ++task_counter << "\n";
            generatePrimes(100000);
            return nullptr;
        }
      );
    task1->setForwardResult(false);

    FileOpenTask* fro = new FileOpenTask(
        [](void* input, void* output) -> void* {
            FileOpenTaskOutput* fo_output = static_cast<FileOpenTaskOutput*>(output);
            return generateFileReadInput(fo_output->file_fd);
        }
    );
    fro->setInput(new FileOpenTaskInput({"/home/users/devika/os/UserLevelScheduler/log.txt", O_RDONLY | O_DIRECT}));

    FileReadTask* frr = new FileReadTask();

    FileCloseTask* frc = new FileCloseTask(
      [](void* in) -> void* {
        FileTaskInput* ft_input = static_cast<FileTaskInput*>(in);
        FileOpenTaskInputExtention* out = new FileOpenTaskInputExtention("/home/users/devika/os/UserLevelScheduler/logh.txt", O_RDWR | O_CREAT | O_TRUNC | O_DIRECT, 0644 , ft_input);
        return out;
      }
    );
  
    FileOpenTask* fwo = new FileOpenTask(
      [](void* in, void* out) -> void* {
        FileOpenTaskInputExtention* fo_in = static_cast<FileOpenTaskInputExtention*>(in);
        FileOpenTaskOutput* fo_out = static_cast<FileOpenTaskOutput*>(out);
        fo_in->ft_input->file_fd = fo_out->file_fd;
        return fo_in->ft_input;
      }
    );
    FileWriteTask* fwr = new FileWriteTask();
    FileCloseTask* fwc = new FileCloseTask();

    /* IO Intensive task chain */

    Task* start_io = task1->fork();
    Task* task1_fork = start_io;
    for (int i=0; i<100; i++) {
      Task* fro_fork = fro->fork();
      Task* frr_fork = frr->fork();
      Task* frc_fork = frc->fork();
      Task* fwo_fork = fwo->fork();
      Task* fwr_fork = fwr->fork();
      Task* fwc_fork = fwc->fork();

      task1_fork->setNextTasks({fro_fork});
      fro_fork->setNextTasks({frr_fork});
      frr_fork->setNextTasks({frc_fork});
      frc_fork->setNextTasks({fwo_fork});
      fwo_fork->setNextTasks({fwr_fork});
      fwr_fork->setNextTasks({fwc_fork});
      task1_fork = task1->fork();
      fwc_fork->setNextTasks({task1_fork});
    }
    
    /* CPU Intensive task chain */

    Task* start_cpu = task1->fork();
    Task* task1_fork1 = start_cpu;
    for (int i=0; i<2; i++) {
      // Task* fro_fork = fro->fork();
      // Task* frr_fork = frr->fork();
      // Task* frc_fork = frc->fork();
      // Task* fwo_fork = fwo->fork();
      // Task* fwr_fork = fwr->fork();
      // Task* fwc_fork = fwc->fork();
      // task1_fork->setNextTasks({fro_fork});
      // fro_fork->setNextTasks({frr_fork});
      // frr_fork->setNextTasks({frc_fork});
      // frc_fork->setNextTasks({fwo_fork});
      // fwo_fork->setNextTasks({fwr_fork});
      // fwr_fork->setNextTasks({fwc_fork});
      // task1_fork1 = task1->fork();
      // fwc_fork->setNextTasks({task1_fork1});
      for (int i=0; i<100; i++) {
        Task* task1_fork2 = task1->fork();
        task1_fork1->setNextTasks({task1_fork2});
        task1_fork1 = task1_fork2;
      }
    }
    
    coordinator.submit(start_io);
    coordinator.submit(start_cpu);
    coordinator.start();

    return 0;
}
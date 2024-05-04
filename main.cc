#include <iostream>
#include <fcntl.h>
#include <thread>
#include <chrono>
#include <csignal>
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

Task* generateCpuTaskChain(int length) {
    length = length * 2;
    Task* task = new CpuTask(
      [](void*) -> void* { 
          generatePrimes(10000);
          return nullptr;
      }
    );
    task->setForwardResult(false);
    Task* start_task = task;
    for (int i=0; i<length-1; i++) {
        Task* task_fork = task->fork();
        task->setNextTasks({task_fork});
        task = task_fork;
    }
    return start_task;
}


Task* generateIoTaskChain(int length, int file_no) {
    std::string read_file_path = "/home/users/devika/os/UserLevelScheduler/logs/log" + std::to_string(file_no) + ".txt";
    std::string write_file_path = "/home/users/devika/os/UserLevelScheduler/logs/out" + std::to_string(file_no) + ".txt";
    file_no++;
    Task* fro = new FileOpenTask(
        [](void* input, void* output) -> void* {
            FileOpenTaskOutput* fo_output = static_cast<FileOpenTaskOutput*>(output);
            return generateFileReadInput(fo_output->file_fd);
        }
    );
    fro->setInput(new FileOpenTaskInput({read_file_path.c_str(), O_RDONLY | O_DIRECT | O_SYNC}));
    Task* frr = new FileReadTask();
    Task* frc = new FileCloseTask(
        [write_file_path](void* in) -> void* {
            FileTaskInput* ft_input = static_cast<FileTaskInput*>(in);
            FileOpenTaskInputExtention* out = new FileOpenTaskInputExtention(write_file_path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0644, ft_input);
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
    Task* fww = new FileWriteTask(
        [](void* in, void* out) -> void* {
            return out;
        }
    );
    Task* fwc = frc->fork();
    Task* cpu_task = new CpuTask(
        [](void* in) -> void* { 
            generatePrimes(100); 
            return in;
        }
    );

    Task* start_task = fro;
    fro->setNextTasks({frr});
    frr->setNextTasks({frc});
    frc->setNextTasks({fwo});
    fwo->setNextTasks({fww});
    for (int i=0; i<length-1; i++) {
        fww->setNextTasks({cpu_task});
        Task* fww_fork = fww->fork();
        Task* cpu_task_fork = cpu_task->fork();
        cpu_task->setNextTasks({fww_fork});
        fww = fww_fork;
        cpu_task = cpu_task_fork;
    }
    fww->setNextTasks({fwc});
    return start_task;
}

Coordinator coordinator;

void sigint_handler(int signal) {
    coordinator.stop();
    std::exit(EXIT_SUCCESS); 
}

void singleIoCpuTasksBenchmark() {
    // Use 1 scheduler
    Task *task1 = generateIoTaskChain(20, 0);
    task1->setGroup("io");
    Task *task2 = generateCpuTaskChain(20);
    task2->setGroup("cpu");
    coordinator.submit(task1);
    coordinator.submit(task2);
}

void singleHighLowPriorityIoTasksBenchmark() {
    // Use 1 scheduler
    Task *task1 = generateIoTaskChain(20, 0);
    task1->setGroup("io-low");
    Task *task2 = generateIoTaskChain(20, 1);
    task2->setGroup("io-high");
    task2->setNiceness(-20);
    coordinator.submit(task1);
    coordinator.submit(task2);
}

void singleHighLowPriorityCpuTasksBenchmark() {
    // Use 1 scheduler
    Task *task1 = generateCpuTaskChain(20);
    task1->setGroup("cpu-low");
    task1->setNiceness(20);
    Task *task2 = generateCpuTaskChain(20);
    task2->setGroup("cpu-high");
    task2->setNiceness(-20);
    coordinator.submit(task1);
    coordinator.submit(task2);
}

void multipleIoCpuTasksBenchmark() {
    // Use 1 scheduler
    for (int i=0; i<10; i++) {
        Task *task = generateIoTaskChain(20, i);
        task->setGroup("multi-io");
        coordinator.submit(task);
    }
    for (int i=0; i<10; i++) {
        Task *task = generateCpuTaskChain(20);
        task->setGroup("multi-cpu");
        coordinator.submit(task);
    }
}

void multipleHighLowPriorityCpuTasksBenchmark() {
    // Use 1 scheduler
    for (int i=0; i<10; i++) {
        Task *task = generateCpuTaskChain(20);
        task->setGroup("multi-cpu-high");
        task->setNiceness(-20);
        coordinator.submit(task);
    }
    for (int i=0; i<10; i++) {
        Task *task = generateCpuTaskChain(20);
        task->setGroup("multi-cpu-low");
        task->setNiceness(20);
        coordinator.submit(task);
    }
}

void multipleHighLowPriorityIoTasksBenchmark() {
    // Use 1 scheduler
    for (int i=0; i<10; i++) {
        Task *task = generateIoTaskChain(20, i);
        task->setGroup("multi-io-high");
        task->setNiceness(-20);
        coordinator.submit(task);
    }
    for (int i=0; i<10; i++) {
        Task *task = generateIoTaskChain(20, i+10);
        task->setGroup("multi-io-low");
        task->setNiceness(20);
        coordinator.submit(task);
    }
}

void multipleSchedulerMultiHighLowPriorityCpuTasksBenchmark() {
    for (int i=0; i<128; i++) {
        Task *task = generateCpuTaskChain(20);
        task->setGroup("multi-sch-cpu-high");
        task->setNiceness(-20);
        coordinator.submit(task);
    }
    for (int i=0; i<128; i++) {
        Task *task = generateCpuTaskChain(20);
        task->setGroup("multi-sch-cpu-low");
        task->setNiceness(20);
        coordinator.submit(task);
    }
}

void multipleSchedulerMultiHighLowPriorityIoTasksBenchmark() {
    for (int i=0; i<128; i++) {
        Task *task = generateIoTaskChain(20, i);
        task->setGroup("multi-sch-io-high");
        task->setNiceness(-20);
        coordinator.submit(task);
    }
    for (int i=0; i<128; i++) {
        Task *task = generateIoTaskChain(20, i+128);
        task->setGroup("multi-sch-io-low");
        task->setNiceness(20);
        coordinator.submit(task);
    }
}

void multipleSchedulerMultiCpuTasksBenchmark() {
    // Use 32 schedulers
    for (int i=0; i<256; i++) {
        Task *task = generateCpuTaskChain(20 * 20);
        task->setGroup("multi-sch-cpu-0");
        coordinator.submit(task);
    }
}

void multipleSchedulerMultiIoTasksBenchmark() {
    // Use 32 schedulers
    for (int i=0; i<256; i++) {
        Task *task = generateIoTaskChain(20, i);
        task->setGroup("multi-sch-io-0");
        coordinator.submit(task);
    }
}

void multipleSchedulerMultiIoCpuTasksBenchmark() {
    // Use 32 schedulers
    for (int i=0; i<256; i++) {
        Task *task = generateIoTaskChain(20, i);
        task->setGroup("multi-sch-io");
        coordinator.submit(task);
    }
    for (int i=0; i<256; i++) {
        Task *task = generateCpuTaskChain(20 * 20);
        task->setGroup("multi-sch-cpu");
        coordinator.submit(task);
    }
}

void multipleSchedulerMultiCpuCpuTasksBenchmark() {
    // Use 32 schedulers
    for (int i=0; i<256; i++) {
        Task *task = generateCpuTaskChain(20 * 20);
        task->setGroup("multi-sch-cpu-1");
        coordinator.submit(task);
    }
    for (int i=0; i<256; i++) {
        Task *task = generateCpuTaskChain(20 * 20);
        task->setGroup("multi-sch-cpu-2");
        coordinator.submit(task);
    }
}

void multipleSchedulerMultiIoIoTasksBenchmark() {
    // Use 32 schedulers
    for (int i=0; i<256; i++) {
        Task *task = generateIoTaskChain(20, i);
        task->setGroup("multi-sch-io-1");
        coordinator.submit(task);
    }
    for (int i=0; i<256; i++) {
        Task *task = generateIoTaskChain(20, i+256);
        task->setGroup("multi-sch-io-2");
        coordinator.submit(task);
    }
}

void loadBalanceCpuTasksBenchmark() {
    // Use 8 schedulers
    for (int i=0; i<256; i++) {
        Task *task = generateCpuTaskChain(20*20*5);
        task->setGroup("load-balance-cpu");
        coordinator.submit(task);
    }
}

void interactivitySingleIoCpuTasksBenchmark() {
    // Use 1 scheduler
    Task *task1 = generateIoTaskChain(20, 0);
    task1->setGroup("interactivity-sch-io-0");
    coordinator.submit(task1);
    Task *task2 = generateCpuTaskChain(20);
    task2->setGroup("interactivity-sch-cpu-0");
    coordinator.submit(task2);
}

void coreCountBenchmark() {
    // Use 4,8.16,32,64,128 schedulers
    for (int i=0; i<128; i++) {
        Task *task = generateIoTaskChain(20, i);
        coordinator.submit(task);
    }
    for (int i=0; i<128; i++) {
        Task *task = generateCpuTaskChain(20 * 20);
        coordinator.submit(task);
    }
}

void threadMigrationBenchmark() {
    // Use 32 schedulers
    for (int i=0; i<32; i++) {
        Task *task = generateCpuTaskChain(20 * 20);
        coordinator.submit(task);
    }
}

int main() {
    // singleIoCpuTasks(); // 5 seconds
    // singleHighLowPriorityIoTasksBenchmark(); // 5 seconds
    // singleHighLowPriorityCpuTasksBenchmark(); // 5 seconds

    // multipleIoCpuTasksBenchmark(); // 30 seconds
    // multipleHighLowPriorityIoTasksBenchmark(); // 30 seconds
    // multipleHighLowPriorityCpuTasksBenchmark(); // 30 seconds

    // loadBalanceCpuTasksBenchmark(); // 5 minutes

    // interactivitySingleIoCpuTasksBenchmark(); // 5 seconds

    // multipleSchedulerMultiCpuTasksBenchmark();
    // multipleSchedulerMultiIoTasksBenchmark();
    // multipleSchedulerMultiIoCpuTasksBenchmark();
    // multipleSchedulerMultiCpuCpuTasksBenchmark();
    // multipleSchedulerMultiIoIoTasksBenchmark();

    // coreCountBenchmark();

    // threadMigrationBenchmark();

    std::signal(SIGINT, sigint_handler);
    coordinator.start();
    return 0;
}

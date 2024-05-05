#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <unistd.h>
#include <stdexcept>
#include <thread>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>
#include <liburing.h>
#include <linux/fs.h>
#include "logger.h"
#include "logger.cc"


#define QUEUE_DEPTH 1024
#define BLOCK_SZ    1024

struct FileTaskInput {
    int file_fd;
    off_t file_size;
    int blocks;
    struct iovec iovecs[];
};

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

FileTaskInput* readFile(int fd) {
    off_t file_size = get_file_size(fd);
    off_t bytes_remaining = file_size;
    int current_block = 0;
    int blocks = (int) file_size / BLOCK_SZ;
    if (file_size % BLOCK_SZ) blocks++;
    FileTaskInput* ft_input = (FileTaskInput*) malloc(sizeof(FileTaskInput) + (sizeof(struct iovec)*  blocks));
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
    ft_input->file_fd = fd;
    ft_input->file_size = file_size;
    ft_input->blocks = blocks;
    return ft_input;
}

void generateCpuTaskChain(int length) {
    length = length * 2;
    for (int i=0; i<length; i++) {
        generatePrimes(10000);
    }
}

void generateIoTaskChain(int length, int file_no) {
    #ifdef ENABLE_THREAD_MIGRATION_METRICS
    int core = sched_getcpu();
    #endif
    struct io_uring ring;
    int ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
    if (ret < 0) {
        throw std::runtime_error("Error: Unable to initialize IO uring");
    }
    std::string read_file_path = "/home/users/devika/os/UserLevelScheduler/logs/log" + std::to_string(file_no) + ".txt";
    std::string write_file_path = "/home/users/devika/os/UserLevelScheduler/logs/out" + std::to_string(file_no) + ".txt";
    
    int fd = open(read_file_path.c_str(), O_RDONLY | O_DIRECT | O_SYNC);
    if (fd == -1) {
        throw std::runtime_error("Error: Failed to open input file.");
    }
    FileTaskInput* ft_input = readFile(fd);
    
    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    io_uring_prep_readv(sqe, ft_input->file_fd, ft_input->iovecs, ft_input->blocks, 0);
    io_uring_sqe_set_data(sqe, ft_input);
    ret = io_uring_submit_and_wait(&ring, 1);
    if (ret < 0) {
        throw std::runtime_error(std::string("Error: File read submission failed: ") + std::to_string(ret));
    }
    io_uring_cq_advance(&ring, 1);

    close(fd);

    fd = open(write_file_path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0644);
    if (fd == -1) {
        throw std::runtime_error("Error: Failed to open input file.");
    }
    ft_input->file_fd = fd;
    #ifdef ENABLE_THREAD_MIGRATION_METRICS
    int curr_core = sched_getcpu();
    if (core != curr_core) {
        logger.info("Migrated core %d", curr_core);
        core = curr_core;
    }
    #endif
    for (int i=0; i<length; i++) {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        io_uring_prep_writev(sqe, ft_input->file_fd, ft_input->iovecs, ft_input->blocks, 0);
        io_uring_sqe_set_data(sqe, ft_input);
        int ret = io_uring_submit_and_wait(&ring, 1);
        if (ret < 0) {
            throw std::runtime_error(std::string("Error: File read submission failed: ") + std::to_string(ret));
        }
        io_uring_cq_advance(&ring, 1);
        #ifdef ENABLE_THREAD_MIGRATION_METRICS
        int curr_core = sched_getcpu();
        if (core != curr_core) {
            logger.info("Migrated core %d", curr_core);
            core = curr_core;
        }
        #endif
        generatePrimes(100);
        #ifdef ENABLE_THREAD_MIGRATION_METRICS
        int curr_core = sched_getcpu();
        if (core != curr_core) {
            logger.info("Migrated core %d", curr_core);
            core = curr_core;
        }
        #endif
    }
    io_uring_queue_exit(&ring);
}

void multipleSchedulerMultiCpuTasksBenchmark() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run] {
            generateCpuTaskChain(20 * 20);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void multipleSchedulerMultiIoTasksBenchmark() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run,i] {
            generateIoTaskChain(20, i);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void multipleSchedulerMultiIoCpuTasksBenchmark() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run,i] {
            generateIoTaskChain(20, i);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run] {
            generateCpuTaskChain(20 * 20);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void multipleSchedulerMultiCpuCpuTasksBenchmark() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run] {
            generateCpuTaskChain(20 * 20);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run] {
            generateCpuTaskChain(20 * 20);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void multipleSchedulerMultiIoIoTasksBenchmark() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run,i] {
            generateIoTaskChain(20, i);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run,i] {
            generateIoTaskChain(20, i+256);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

std::chrono::steady_clock::time_point threadMigrationBenchmark() {
    std::vector<std::thread> threads;
    for (int i=0; i<512; i++) {
        threads.emplace_back([] {
            logger.info("Started\n");
            generateIoTaskChain(20, 0);
        });
    }
    auto start = std::chrono::steady_clock::now();
    for (std::thread& thread : threads) {
        thread.join();
    }
    return start;
}

int main() {
    auto start = std::chrono::steady_clock::now();

    // multipleSchedulerMultiCpuTasksBenchmark();
    // multipleSchedulerMultiIoTasksBenchmark();
    // multipleSchedulerMultiIoCpuTasksBenchmark(); 
    // multipleSchedulerMultiCpuCpuTasksBenchmark();
    // multipleSchedulerMultiIoIoTasksBenchmark(); 

    // start = threadMigrationBenchmark();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    logger.info("Total task run time is %d milliseconds", duration.count());
    return 0;
}

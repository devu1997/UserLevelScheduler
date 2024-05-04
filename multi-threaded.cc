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
#include <linux/fs.h>
#include "logger.h"
#include "logger.cc"


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

FileTaskInput* readFile(std::string read_file_path) {
    int fd = open(read_file_path.c_str(), O_RDONLY | O_DIRECT | O_SYNC);
    if (fd == -1) {
        throw std::runtime_error("Error: Failed to open input file.");
    }
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
        pread(fd, ft_input->iovecs[current_block].iov_base, ft_input->iovecs[current_block].iov_len, BLOCK_SZ * current_block);
        current_block++;
        bytes_remaining -= bytes_to_read;
    }
    ft_input->file_size = file_size;
    ft_input->blocks = blocks;
    close(fd);
    return ft_input;
}

void writeFile(std::string write_file_path, FileTaskInput* ft_input) {
    for (int current_block=0; current_block<ft_input->blocks; current_block++) {
        pwrite(ft_input->file_fd, ft_input->iovecs[current_block].iov_base, ft_input->iovecs[current_block].iov_len, BLOCK_SZ * current_block);
    }
}

void generateCpuTaskChain(int length) {
    length = length * 2;
    for (int i=0; i<length; i++) {
        generatePrimes(10000);
    }
}

void generateIoTaskChain(int length, int file_no) {
    std::string read_file_path = "/home/users/devika/os/UserLevelScheduler/logs/log" + std::to_string(file_no) + ".txt";
    std::string write_file_path = "/home/users/devika/os/UserLevelScheduler/logs/out" + std::to_string(file_no) + ".txt";
    FileTaskInput* ft_input = readFile(read_file_path);
    int fd = open(write_file_path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_DIRECT | O_SYNC, 0644);
    if (fd == -1) {
        throw std::runtime_error("Error: Failed to open input file.");
    }
    ft_input->file_fd = fd;
    for (int i=0; i<length; i++) {
        writeFile(write_file_path, ft_input);
        generatePrimes(100);
    }
}


void multipleSchedulerMultiCpuTasks() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run] {
            // Set CPU affinity
            #ifdef ENABLE_PINNING
            #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);       
            CPU_SET(core_to_run, &cpuset);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
            }
            #endif
            #endif
            generateCpuTaskChain(20 * 20);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void multipleSchedulerMultiIoTasks() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run,i] {
            // Set CPU affinity
            #ifdef ENABLE_PINNING
            #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);       
            CPU_SET(core_to_run, &cpuset);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
            }
            #endif
            #endif
            generateIoTaskChain(20, i);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void multipleSchedulerMultiIoCpuTasks() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run,i] {
            // Set CPU affinity
            #ifdef ENABLE_PINNING
            #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);       
            CPU_SET(core_to_run, &cpuset);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
            }
            #endif
            #endif
            generateIoTaskChain(20, i);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run] {
            // Set CPU affinity
            #ifdef ENABLE_PINNING
            #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);       
            CPU_SET(core_to_run, &cpuset);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
            }
            #endif
            #endif
            generateCpuTaskChain(20 * 20);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void multipleSchedulerMultiCpuCpuTasks() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run] {
            // Set CPU affinity
            #ifdef ENABLE_PINNING
            #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);       
            CPU_SET(core_to_run, &cpuset);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
            }
            #endif
            #endif
            generateCpuTaskChain(20 * 20);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run] {
            // Set CPU affinity
            #ifdef ENABLE_PINNING
            #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);       
            CPU_SET(core_to_run, &cpuset);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
            }
            #endif
            #endif
            generateCpuTaskChain(20 * 20);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void multipleSchedulerMultiIoIoTasks() {
    std::vector<std::thread> threads;
    int core_to_run = 0;
    int max_cores = std::thread::hardware_concurrency();
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run,i] {
            // Set CPU affinity
            #ifdef ENABLE_PINNING
            #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);       
            CPU_SET(core_to_run, &cpuset);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
            }
            #endif
            #endif
            generateIoTaskChain(20, i);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (int i=0; i<256; i++) {
        threads.emplace_back([core_to_run,i] {
            // Set CPU affinity
            #ifdef ENABLE_PINNING
            #ifdef __linux__
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);       
            CPU_SET(core_to_run, &cpuset);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
                throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + std::strerror(errno));
            }
            #endif
            #endif
            generateIoTaskChain(20, i+256);
        });
        core_to_run = (core_to_run + 1) % max_cores;
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

int main() {
    auto start = std::chrono::steady_clock::now();
    // multipleSchedulerMultiCpuTasks();
    // multipleSchedulerMultiIoTasks();
    // multipleSchedulerMultiIoCpuTasks(); 
    // multipleSchedulerMultiCpuCpuTasks();
    // multipleSchedulerMultiIoIoTasks(); 
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    logger.info("Total task run time is %d milliseconds", duration.count());
    return 0;
}

/*
Copyright (c) 2020 Erik Rigtorp <erik@rigtorp.se>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#pragma once

#include <vector>
#include <atomic>
#include "logger.h"


template<class T>
class RingBuffer {
private:
    #ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t kCacheLineSize = std::hardware_destructive_interference_size;
    #else
    static constexpr size_t kCacheLineSize = 64;
    #endif
    // Padding to avoid false sharing between slots_ and adjacent allocations
    static constexpr size_t kPadding = (kCacheLineSize - 1) / sizeof(T) + 1;

    // Align to cache line size in order to avoid false sharing
    // readIdxCache and writeIdxCache is used to reduce the amount of cache
    // coherency traffic
    alignas(kCacheLineSize) std::atomic<size_t> writeIdx = {0};
    alignas(kCacheLineSize) std::atomic<size_t> readIdx = {0};

    size_t capacity;
    T *slots_;

public:
    RingBuffer(size_t capacity = 10000);

    void enque(T t);
    T deque();
    bool empty();
};

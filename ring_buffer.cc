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

#include "ring_buffer.h"
#include <stdexcept>
#include <memory>

template<class T>
RingBuffer<T>::RingBuffer(size_t capacity) {
    if (capacity < 1) {
        capacity = 1;
    }
    capacity++; // Needs one slack element
    if (capacity > SIZE_MAX - 2 * kPadding) {
        capacity = SIZE_MAX - 2 * kPadding;
    }
    this->capacity = capacity;
    this->slots_ = static_cast<T*>(std::malloc((capacity + 2 * kPadding) * sizeof(T)));
}

template<class T>
void RingBuffer<T>::enque(T t) {
    auto const writeIdx_ = writeIdx.load(std::memory_order_relaxed);
    auto nextWriteIdx = writeIdx_ + 1;
    if (nextWriteIdx == capacity) {
        nextWriteIdx = 0;
    }
    if (nextWriteIdx == readIdx.load(std::memory_order_acquire)) {
        throw std::overflow_error("Buffer is full");
    }
    slots_[writeIdx_ + kPadding] = t;
    writeIdx.store(nextWriteIdx, std::memory_order_release);
}

template<class T>
T RingBuffer<T>::deque() {
    auto const readIdx_ = readIdx.load(std::memory_order_relaxed);
    if (readIdx_ == writeIdx.load(std::memory_order_acquire)) {
        logger.error("Empty? %d", empty());
        throw std::underflow_error("Buffer is empty");
    }
    auto nextReadIdx = readIdx_ + 1;
    if (nextReadIdx == capacity) {
        nextReadIdx = 0;
    }
    T t = slots_[readIdx_ + kPadding];
    readIdx.store(nextReadIdx, std::memory_order_release);
    return t;
}

template<class T>
bool RingBuffer<T>::empty() {
    return readIdx.load(std::memory_order_acquire) == writeIdx.load(std::memory_order_acquire);
}

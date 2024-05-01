/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * Modified by Devika Sudheer
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>


#if defined(__arm__)
#define ARM 1
#else
#define ARM 0
#endif

#if defined(__s390x__)
#define S390X 1
#else
#define S390X 0
#endif

constexpr std::size_t hardware_destructive_interference_size = (ARM == 1 || S390X == 1) ? 64 : 128;

template <class T>
struct ProducerConsumerQueue {

 private:
  using AtomicIndex = std::atomic<unsigned int>;

  char pad0_[hardware_destructive_interference_size];
  const uint32_t capacity;
  T* const records;

  alignas(hardware_destructive_interference_size) AtomicIndex readIndex;
  alignas(hardware_destructive_interference_size) AtomicIndex writeIndex;

  char pad1_[hardware_destructive_interference_size - sizeof(AtomicIndex)];

 public:
  typedef T value_type;

  ProducerConsumerQueue(const ProducerConsumerQueue&) = delete;
  ProducerConsumerQueue& operator=(const ProducerConsumerQueue&) = delete;

  // capacity must be >= 2.
  // Also, note that the number of usable slots in the queue at any
  // given time is actually (capacity-1), so if you start with an empty queue,
  // full() will return true after capacity-1 insertions.
  explicit ProducerConsumerQueue(uint32_t capacity = 1000)
      : capacity(capacity),
        records(static_cast<T*>(std::malloc(sizeof(T) * capacity))),
        readIndex(0),
        writeIndex(0) {
    assert(capacity >= 2);
    if (!records) {
      throw std::bad_alloc();
    }
  }

  bool enque(T t) {
    auto const currentWrite = writeIndex.load(std::memory_order_relaxed);
    auto nextRecord = currentWrite + 1;
    if (nextRecord == capacity) {
      nextRecord = 0;
    }
    assert(nextRecord != readIndex.load(std::memory_order_acquire));
    records[currentWrite] = t;
    writeIndex.store(nextRecord, std::memory_order_release);
    return true;
  }

  T deque() {
    auto const currentRead = readIndex.load(std::memory_order_relaxed);
    assert(currentRead != writeIndex.load(std::memory_order_acquire));
    auto nextRecord = currentRead + 1;
    if (nextRecord == capacity) {
      nextRecord = 0;
    }
    T t = records[currentRead];
    readIndex.store(nextRecord, std::memory_order_release);
    return t;
  }

  bool empty() const {
    return readIndex.load(std::memory_order_acquire) ==
        writeIndex.load(std::memory_order_acquire);
  }

  bool full() const {
    auto nextRecord = writeIndex.load(std::memory_order_acquire) + 1;
    if (nextRecord == capacity) {
      nextRecord = 0;
    }
    return nextRecord == readIndex.load(std::memory_order_acquire);
  }

};

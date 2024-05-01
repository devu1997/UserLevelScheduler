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

    const uint32_t capacity;
    T* const records;

    alignas(hardware_destructive_interference_size) AtomicIndex readIndex;
    alignas(hardware_destructive_interference_size) AtomicIndex writeIndex;

public:
    typedef T value_type;

    ProducerConsumerQueue(const ProducerConsumerQueue&) = delete;
    ProducerConsumerQueue& operator=(const ProducerConsumerQueue&) = delete;
    ProducerConsumerQueue(uint32_t capacity = 1000);

    bool enque(T t);
    T deque();
    bool empty() const;
    bool full() const;
};

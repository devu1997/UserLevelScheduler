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

#include "producer_consumer_queue.h"

template <class T>
ProducerConsumerQueue<T>::ProducerConsumerQueue(uint32_t capacity)
: capacity(capacity),
records(static_cast<T*>(std::malloc(sizeof(T) * capacity))),
readIndex(0),
writeIndex(0) {
    assert(capacity >= 2);
    if (!records) {
        throw std::bad_alloc();
    }
}

template <class T>
bool ProducerConsumerQueue<T>::enque(T t) {
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

template <class T>
T ProducerConsumerQueue<T>::deque() {
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

template <class T>
bool ProducerConsumerQueue<T>::empty() const {
    return readIndex.load(std::memory_order_acquire) ==
        writeIndex.load(std::memory_order_acquire);
}

template <class T>
bool ProducerConsumerQueue<T>::full() const {
    auto nextRecord = writeIndex.load(std::memory_order_acquire) + 1;
    if (nextRecord == capacity) {
      nextRecord = 0;
    }
    return nextRecord == readIndex.load(std::memory_order_acquire);
}

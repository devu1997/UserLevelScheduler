#include "ring_buffer.h"
#include <stdexcept>

template<class T>
RingBuffer<T>::RingBuffer(size_t capacity) : buffer(capacity), capacity(capacity), size_(0), head(0), tail(0) {}

template<class T>
bool RingBuffer<T>::empty() const {
    return size_ == 0;
}

template<class T>
bool RingBuffer<T>::full() const {
    return size_ == capacity;
}

template<class T>
size_t RingBuffer<T>::size() const {
    return size_;
}

template<class T>
void RingBuffer<T>::enque(T t) {
    if (full()) {
        throw std::overflow_error("Buffer is full");
    }
    buffer[tail] = t;
    tail = (tail + 1) % capacity;
    size_++;
}

template<class T>
T RingBuffer<T>::deque() {
    if (empty()) {
        throw std::underflow_error("Buffer is empty");
    }
    T element = buffer[head];
    head = (head + 1) % capacity;
    size_--;
    return element;
}

#pragma once

#include <vector>

template<class T>
class RingBuffer {
private:
    std::vector<T> buffer; 
    size_t capacity;        
    size_t size_;
    int head;        
    int tail;        

public:
    RingBuffer(size_t capacity = 1000);

    bool empty() const;
    bool full() const;
    size_t size() const;
    void enque(T t);
    T deque();
};

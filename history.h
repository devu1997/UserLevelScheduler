#pragma once

#include <iostream>
#include <deque>
#include <chrono>


enum class EventType {
    CPU,
    IO
};

struct Event {
    EventType type;
    std::chrono::nanoseconds duration;
};

class History {
private:
    std::deque<Event> events{};

public:
    std::chrono::nanoseconds run_time = std::chrono::nanoseconds(0);
    std::chrono::nanoseconds sleep_time = std::chrono::nanoseconds(0);
  
    History();
    ~History();

    void addEvent(Event event);

};


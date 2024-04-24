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
    std::chrono::milliseconds duration;
};

class History {
private:
    std::deque<Event> events{};

public:
    std::chrono::milliseconds run_time = std::chrono::milliseconds(0);
    std::chrono::milliseconds sleep_time = std::chrono::milliseconds(0);
  
    History();
    ~History();

    void addEvent(Event event);

};


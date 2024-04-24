#include "history.h"

#define HISTORY_THRESHOLD std::chrono::seconds(5)

History::History() {}

History::~History() {}

void History::addEvent(Event event) {
    events.push_back(event);
    if (event.type == EventType::CPU) {
        run_time = run_time + event.duration;
    } else {
        sleep_time = sleep_time + event.duration;
    }
    while (!events.empty() && run_time + sleep_time > HISTORY_THRESHOLD) {
        Event lastEvent = events.front();
        if (run_time + sleep_time - lastEvent.duration < HISTORY_THRESHOLD) {
            auto extra_duration = lastEvent.duration - (run_time + sleep_time - HISTORY_THRESHOLD);
            lastEvent.duration -= extra_duration;
            if (lastEvent.type == EventType::CPU) {
                run_time -= extra_duration;
            } else {
                sleep_time -= extra_duration;
            }
        } else {
            if (lastEvent.type == EventType::CPU) {
                run_time -= lastEvent.duration;
            } else {
                sleep_time -= lastEvent.duration;
            }
            events.pop_front();
        }
    }
}

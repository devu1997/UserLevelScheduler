#pragma once

#include <liburing.h>
#include "filetasks.h"


class Scheduler;

class FileScheduler {
private:
    Scheduler* scheduler;
    struct io_uring ring;
    int pending_requests = 0;

public:
    FileScheduler();
    ~FileScheduler();

    void submit(AsyncFileReadTask *task);
    void process_completed();
    void setScheduler(Scheduler*schedulers);
};

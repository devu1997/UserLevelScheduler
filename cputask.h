#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <any>
#include <string.h>
#include "task.h"

class CpuTask : public Task {

public:
    using Task::Task;

    void* process() override;
    Task* fork() override;
    
};

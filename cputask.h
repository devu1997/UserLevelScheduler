#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <any>
#include <string.h>
#include "task.h"

class CpuTask : public Task {
private:
    std::function<void*(void*)> func;

public:
    CpuTask(std::function<void*(void*)> func = [](void*) { return nullptr; });

    void* process() override;
    Task* fork() override;
    
};

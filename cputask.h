#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <any>
#include <string.h>
#include "task.h"

class CpuTask : public Task {

public:
    CpuTask(std::function<void*()> func, bool forward_result = false);

    void* process() override;
    
};

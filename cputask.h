#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <any>
#include <string.h>
#include "task.h"

struct CpuTaskInput {
    std::vector<std::any> args;
    std::function<void*(std::vector<std::any>)> func;
};

class CpuTask : public Task {

public:
    CpuTask(bool forward_result = false);

    void* process() override;
    
};

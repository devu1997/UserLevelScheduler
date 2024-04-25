#include <sstream>
#include <stdexcept>
#include <chrono>
#include "task.h"


int Task::last_task_id = 0;

Task::Task() {
    this->id = Task::generate_task_id();
}

void Task::setForwardResult(bool forward_result) {
    this->forward_result = forward_result;
}

void Task::setInput(void* input) {
    this->input = input;
}

void Task::setNextTasks(std::vector<Task*> next_tasks) {
    this->next_tasks = next_tasks;
}

void Task::setHistory(History history) {
    this->history = history;
}

void Task::setExecutionMode(TaskExecutionMode exec_mode) {
    this->exec_mode = exec_mode;
}

void Task::setNiceness(int niceness) {
    if (niceness < MIN_NICENESS || niceness > MAX_NICENESS) {
        std::ostringstream error_msg;
        error_msg << "Error: Niceness value must be between " << MIN_NICENESS << " to " << MAX_NICENESS;
        throw std::runtime_error(error_msg.str()); 
    }
    this->niceness = niceness;
}

void Task::setTicks(long ticks, long ftick, long ltick) {
    this->ticks = ticks;
    this->ftick = ftick;
    this->ltick = ltick;
}

void Task::copy(Task* task) {
    task->setInput(this->input);
    task->setForwardResult(this->forward_result);
    task->setNextTasks(this->next_tasks);
    task->setHistory(this->history);
    task->setNiceness(this->niceness);
    task->setTicks(this->ticks, this->ftick, this->ltick);
}

int Task::getInteractivityPenality() {
    if (history.sleep_time == std::chrono::milliseconds::zero() && history.run_time == std::chrono::milliseconds::zero()) {
        return SCHED_INTERACT_HALF; // Initially set the task as batch so that non-interactive tasks will not starve interactive tasks
    }
    int penality = 0;
    if (history.sleep_time > history.run_time) {
        penality = (SCHED_INTERACT_HALF * history.run_time) / history.sleep_time;
    } else if (history.sleep_time < history.run_time) {
        penality = SCHED_INTERACT_HALF + ((SCHED_INTERACT_HALF * history.sleep_time) / history.run_time);
    } else {
        return SCHED_INTERACT_HALF;
    }
    return penality;
}

void Task::updateCpuUtilization(std::chrono::steady_clock::duration duration, bool run) {
    long t = (duration.count() * std::chrono::steady_clock::period::den) / std::chrono::steady_clock::period::num;
    std::cout<<" ticks: "<<t<<" task_ticks: "<<ticks<<" ftick: "<<ftick<<" ltick: "<<ltick<<"\n";
    
    if ((u_int)(t - ltick) >= SCHED_TICK_TARG) {
        ticks = 0;
        ftick = t - SCHED_TICK_TARG;
    } else if (t - ftick >= SCHED_TICK_MAX) {
    std::cout<<" ticks: "<<t<<" task_ticks: "<<ticks<<" ftick: "<<ftick<<" ltick: "<<ltick<<"\n";
        ticks = (ticks / (ltick - ftick)) * (ltick - (t - SCHED_TICK_TARG));
        ftick = t - SCHED_TICK_TARG;
    }
    std::cout<<" ticks: "<<t<<" task_ticks: "<<ticks<<" ftick: "<<ftick<<" ltick: "<<ltick<<"\n";
    if (run) {
        ticks += (t - ltick) << SCHED_TICK_SHIFT;
    }
    ltick = t;
    std::cout<<" ticks: "<<t<<" task_ticks: "<<ticks<<" ftick: "<<ftick<<" ltick: "<<ltick<<"\n";
}

int Task::getPriority() {
    int penality = getInteractivityPenality();
    int score = std::max(0, penality + niceness);
    return score;
    // int priority = 0;
    // if (score < SCHED_INTERACT_THRESH) {
    //     priority = PRI_MIN_INTERACT;
    //     priority += (PRI_MAX_INTERACT - PRI_MIN_INTERACT + 1) * score / SCHED_INTERACT_THRESH;
    //     if (priority < PRI_MIN_INTERACT || priority > PRI_MAX_INTERACT) {
    //         throw std::runtime_error("Error: Interactive priority out of range");
    //     }
    // } else {
    //     priority = SCHED_PRI_MIN;
    //     if (ticks) {
    //         priority += std::min((int) SCHED_PRI_TICKS(ticks, ltick, ftick), SCHED_PRI_RANGE - 1);
    //         priority += niceness;
    //         if (priority < PRI_MIN_BATCH || priority > PRI_MAX_BATCH) {
    //             throw std::runtime_error("Error: Batch priority out of range");
    //         }
    //     }
    // }
    // return priority;
}

int Task::generate_task_id() {
    return ++last_task_id;
}

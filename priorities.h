#define PRI_MIN_INTERACT 88
#define PRI_MAX_INTERACT 135
#define PRI_INTERACT_RANGE (PRI_MAX_INTERACT + 1 - PRI_MIN_INTERACT)

#define PRI_MIN_BATCH  136
#define PRI_MAX_BATCH  223

#define MIN_NICENESS     -20
#define MAX_NICENESS     20
#define DEFAULT_NICENESS 0

#define SCHED_PRI_NRESV (MAX_NICENESS - MIN_NICENESS)
#define SCHED_PRI_NHALF (SCHED_PRI_NRESV / 2)

#define SCHED_PRI_MIN   (PRI_MIN_BATCH + SCHED_PRI_NHALF)
#define SCHED_PRI_MAX   (PRI_MAX_BATCH - SCHED_PRI_NHALF)
#define SCHED_PRI_RANGE (SCHED_PRI_MAX - SCHED_PRI_MIN + 1) 

#define SCHED_INTERACT_MAX      (100)
#define SCHED_INTERACT_HALF     (SCHED_INTERACT_MAX / 2)
#define SCHED_INTERACT_THRESH   (30)

#define roundup(x, y)           ((((x) + ((y) - 1)) / (y)) * (y))

#define hz                                   (std::chrono::steady_clock::period::den / std::chrono::steady_clock::period::num)
#define SCHED_TICK_SECS                      10
#define SCHED_TICK_TARG                      (hz * SCHED_TICK_SECS)
#define SCHED_TICK_MAX                       (SCHED_TICK_TARG + hz)
#define SCHED_TICK_SHIFT                     10
#define SCHED_TICK_HZ(ticks)                 (ticks >> SCHED_TICK_SHIFT)
#define SCHED_TICK_TOTAL(ltick, ftick)       ((ltick - ftick > hz) ? (ltick - ftick) : hz)
#define SCHED_PRI_TICKS(ticks, ltick, ftick) (SCHED_TICK_HZ(ticks) / (roundup(SCHED_TICK_TOTAL(ltick, ftick), SCHED_PRI_RANGE) / SCHED_PRI_RANGE))

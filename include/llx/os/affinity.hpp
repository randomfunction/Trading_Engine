#pragma once

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

namespace llx::util {

inline bool pin_this_thread(int cpu) noexcept {
#if defined(__linux__)
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    return pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0;
#else
    (void)cpu;
    return false;
#endif
}

}  // namespace llx::util

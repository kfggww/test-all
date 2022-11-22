#ifndef _TIMER_POSIX_H_
#define _TIMER_POSIX_H_

#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <map>
#include <mutex>
#include <set>

#include "timer.h"

/**
 * @brief Super class of timers that using POSIX timer API.
 */
class TimerPOSIX : public Timer {
  public:
    TimerPOSIX();
    static int GetRealTimeSignalNo();

  private:
    static void Create();

  private:
    static std::once_flag create_flag_;
};

/**
 * @brief High resolution timer implementation. Each timer of this class can hold no more than one callback, after the
 * current callback is called, the new one can be added.
 */
class TimerHighResolution final : public Timer {
  public:
    TimerHighResolution();
    bool AddCallback(const TimerCallbackEntity &cbe) override;
    bool RemoveCallback(const TimerCallback cb) override;

  private:
    static void *WorkerThreadEntry(void *data);

  private:
    bool created_;
    TimerCallbackEntity cb_entity_;
    int signo_;
    timer_t posix_timerid_;
    pthread_t worker_thread_;
};

/**
 * @brief Timer implementation using POSIX timer API, the resolution of this timer is 500ms for now. The typical usages
 * are as follows:
 *
 * @code{.cpp}
 * // 1. Create a timer
 * TimerNormalResolution timer;
 *
 * // 2. Add some user defined callback to timer
 * timer.AddCallback(...);
 *
 * // 3. Client program can do other stuff now, the callback will be called automaticlly when the deadline arrives
 * @endcode
 */
class TimerNormalResolution final : public TimerPOSIX {
  public:
    TimerNormalResolution();
    virtual ~TimerNormalResolution() = default;

    bool AddCallback(const TimerCallbackEntity &cbe) override;
    bool RemoveCallback(const TimerCallback cb) override;

  private:
    static void Create();

    static void *MainThreadEntry(void *data);
    static void *WorkerThreadEntry(void *data);

    static void EnableTimer();
    static void DisableTimer();

    static void HandleCallbacks();

  private:
    static std::once_flag create_flag_;
    static timer_t posix_timerid_;
    static sigset_t sigset_;
    static bool timer_disabled_;
    static int signo_;

    static volatile bool worker_busy_;
    static pthread_mutex_t worker_busy_lock_;
    static pthread_cond_t worker_busy_cond_;

    static std::set<TimerCallbackEntity> cb_set_;
    static std::map<TimerCallback, TimerCallbackEntity> cb_map_;
    static pthread_mutex_t cb_lock_;

    static pthread_t main_thread_;
    static pthread_t worker_thread_;
};

#endif
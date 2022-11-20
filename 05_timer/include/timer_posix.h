#ifndef _TIMER_POSIX_H_
#define _TIMER_POSIX_H_

#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <map>
#include <set>

#include "timer.h"

/**
 * @brief Timer implementation using POSIX timer API, the resolution of this timer is 500ms for now. The typical usages
 * are as follows:
 *
 * @code{.cpp}
 * // 1. Create a timer
 * TimerPOSIX timer;
 *
 * // 2. Add some user defined callback to timer
 * timer.AddCallback(...);
 *
 * // 3. Client program can do other stuff now, the callback will be called automaticlly when the deadline arrives
 * @endcode
 */
class TimerPOSIX final : public Timer {
  public:
    TimerPOSIX();
    virtual ~TimerPOSIX() = default;

    bool AddCallback(long ms, const TimerCallback cb, void *data) override;
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
    static timer_t posix_timerid_;
    static sigset_t sigset_;
    static bool timer_disabled_;

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
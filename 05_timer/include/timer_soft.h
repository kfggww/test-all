#ifndef _TIMER_SOFT_H_
#define _TIMER_SOFT_H_

#include <map>
#include <pthread.h>
#include <set>

#include "timer.h"

class TimerLowResolution final : public Timer {
  public:
    TimerLowResolution();
    virtual ~TimerLowResolution();

    bool AddCallback(const TimerCallbackEntity &cbe) override;
    bool RemoveCallback(const TimerCallback cb) override;

  private:
    TimerCallbackEntity GetFirstCBentity() const;
    struct timespec GetSleepInterval(const TimerCallbackEntity &cbe) const;

    static void *WorkerThreadEntry(void *);

  private:
    pthread_t worker_thread_;

    std::set<TimerCallbackEntity> cbe_set_;
    std::map<TimerCallback, TimerCallbackEntity> cbe_map_;

    pthread_mutex_t cbe_lock_;
    pthread_cond_t cbe_changed_cond_;
    bool cbe_changed_;

    bool timer_stoped_;
};

#endif
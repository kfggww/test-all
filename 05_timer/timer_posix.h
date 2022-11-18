#ifndef _TIMER_POSIX_H_
#define _TIMER_POSIX_H_

#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <map>
#include <set>

#include "timer.h"

class TimerPOSIX final : public Timer {
  public:
    TimerPOSIX();
    virtual ~TimerPOSIX() = default;

    bool AddCallback(long ms, const TimerCallback cb, void *data) override;
    bool RemoveCallback(const TimerCallback cb) override;

  private:
    static void Create();

    static void *MainThreadEntry(void *data);
    static void *WorkerThreadEntry(void *data);

    static void EnableTimer();
    static void DisableTimer();

    static void HandleCallbacks();

    class CBEntity {
      public:
        CBEntity() = default;
        CBEntity(const CBEntity &other) = default;
        CBEntity(const long interval_ms, const TimerCallback cb, void *data);

        bool operator<(const CBEntity &other) const;
        const struct timespec *GetDeadline() const;
        TimerCallback GetCallback() const;
        void *GetData() const;

      private:
        struct timespec deadline_;
        TimerCallback callback_;
        void *data_;
    };

  private:
    static timer_t posix_timerid_;
    static sigset_t sigset_;
    static bool timer_disabled_;

    static volatile bool worker_busy_;
    static pthread_mutex_t worker_busy_lock_;
    static pthread_cond_t worker_busy_cond_;

    static std::set<CBEntity> cb_set_;
    static std::map<TimerCallback, CBEntity> cb_map_;
    static pthread_mutex_t cb_lock_;

    static pthread_t main_thread_;
    static pthread_t worker_thread_;
};

#endif
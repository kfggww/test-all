#include "timer_soft.h"

TimerLowResolution::TimerLowResolution()
    : cbe_changed_(false), timer_stoped_(false) {
    pthread_mutex_init(&cbe_lock_, NULL);
    pthread_cond_init(&cbe_changed_cond_, NULL);

    pthread_create(&worker_thread_, NULL, TimerLowResolution::WorkerThreadEntry,
                   this);
}

TimerLowResolution::~TimerLowResolution() {}

bool TimerLowResolution::AddCallback(const TimerCallbackEntity &cbe) {
    if (!cbe.IsValid())
        return false;

    pthread_mutex_lock(&cbe_lock_);
    TimerCallback cb = cbe.GetCallback();
    if (cbe_map_.find(cb) != cbe_map_.end()) {
        pthread_mutex_unlock(&cbe_lock_);
        return false;
    }

    cbe_map_[cb] = cbe;
    cbe_set_.insert(cbe);

    cbe_changed_ = true;
    pthread_mutex_unlock(&cbe_lock_);
    pthread_cond_signal(&cbe_changed_cond_);

    return true;
}

bool TimerLowResolution::RemoveCallback(const TimerCallback cb) {

    if (nullptr == cb)
        return false;

    pthread_mutex_lock(&cbe_lock_);
    if (cbe_map_.find(cb) == cbe_map_.end()) {
        pthread_mutex_unlock(&cbe_lock_);
        return false;
    }

    TimerCallbackEntity cbe = cbe_map_[cb];
    cbe_set_.erase(cbe);
    cbe_map_.erase(cb);

    cbe_changed_ = true;
    pthread_mutex_unlock(&cbe_lock_);
    pthread_cond_signal(&cbe_changed_cond_);

    return true;
}

void *TimerLowResolution::WorkerThreadEntry(void *data) {
    TimerLowResolution *timer = static_cast<TimerLowResolution *>(data);
    if (nullptr == timer)
        return NULL;

    int rc = 0;
    while (true) {
        TimerCallbackEntity cbe = timer->GetFirstCBentity();
        if (!cbe.IsValid()) {
            continue;
        }
        const struct timespec *tv = cbe.GetDeadline();

        pthread_mutex_lock(&timer->cbe_lock_);
        while (!timer->cbe_changed_ && 0 == rc) {
            rc = pthread_cond_timedwait(&timer->cbe_changed_cond_,
                                        &timer->cbe_lock_, tv);
        }
    }

    return NULL;
}

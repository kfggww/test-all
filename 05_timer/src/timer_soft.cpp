#include <signal.h>

#include "timer_soft.h"

/**
 * @brief Construct low resolution timer. Worker thread is started.
 */
TimerLowResolution::TimerLowResolution()
    : cbe_changed_(false), timer_stoped_(false) {
    pthread_mutex_init(&cbe_lock_, NULL);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    pthread_cond_init(&cbe_changed_cond_, &cond_attr);

    pthread_create(&worker_thread_, NULL, TimerLowResolution::WorkerThreadEntry,
                   this);
}

/**
 * @brief Destruct low resolution timer. Woker thread is cancled.
 */
TimerLowResolution::~TimerLowResolution() { pthread_cancel(worker_thread_); }

/**
 * @brief Add callback entity to the timer.
 *
 * @param cbe: callback entity
 *
 * @return true if success
 */
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

/**
 * @brief Remove callback from timer.
 *
 * @param cb: callback function pointer that has been added before
 *
 * @return true if success
 */
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

/**
 * @brief Get the first callback entity lies on time axis.
 *
 * @return The callback entity with earliest deadline
 */
TimerCallbackEntity TimerLowResolution::GetFirstCallbackEntity() const {
    TimerCallbackEntity cbe(nullptr, nullptr, 0);
    if (cbe_set_.empty())
        return cbe;

    cbe = *cbe_set_.begin();
    return cbe;
}

/**
 * @brief Handle all the expired callbacks.
 */
void TimerLowResolution::HandleCallbacks() {
    struct timespec now;
    auto itr_cbe = cbe_set_.begin();
    while (itr_cbe != cbe_set_.end()) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (LaterThan(&now, itr_cbe->GetDeadline())) {
            TimerCallback cb = itr_cbe->GetCallback();
            void *data = itr_cbe->GetData();
            cb(data);
            cbe_map_.erase(cb);
            itr_cbe = cbe_set_.erase(itr_cbe);
        } else
            break;
    }
}

/**
 * @brief The worker thread entry of low resolution timer.
 *
 * @param data: pointer to a low resolution timer
 *
 * @return NULL
 */
void *TimerLowResolution::WorkerThreadEntry(void *data) {
    TimerLowResolution *timer = static_cast<TimerLowResolution *>(data);
    if (nullptr == timer)
        return NULL;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    int rc = 0;
    while (true) {
        pthread_mutex_lock(&timer->cbe_lock_);

        struct timespec tv;
        TimerCallbackEntity cbe = timer->GetFirstCallbackEntity();
        if (cbe.IsValid())
            tv = *cbe.GetDeadline();
        else {
            clock_gettime(CLOCK_MONOTONIC, &tv);
            tv.tv_sec += 2;
        }

        while (!timer->cbe_changed_ && 0 == rc) {
            rc = pthread_cond_timedwait(&timer->cbe_changed_cond_,
                                        &timer->cbe_lock_, &tv);
        }

        if (timer->cbe_changed_) {
            timer->cbe_changed_ = false;
            pthread_mutex_unlock(&timer->cbe_lock_);
            continue;
        }

        timer->HandleCallbacks();

        pthread_mutex_unlock(&timer->cbe_lock_);
    }

    return NULL;
}

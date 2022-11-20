#include "timer_posix.h"

#include <assert.h>

#include <mutex>

/*Real time signal that POSIX timer uses*/
#define SIG_POSIX_TIMER SIGRTMIN
/*POSIX timer resolution*/
#define RES_POSIX_TIMER 500

static std::once_flag create_flag;

timer_t TimerPOSIX::posix_timerid_;
sigset_t TimerPOSIX::sigset_;
bool TimerPOSIX::timer_disabled_;

volatile bool TimerPOSIX::worker_busy_;
pthread_mutex_t TimerPOSIX::worker_busy_lock_;
pthread_cond_t TimerPOSIX::worker_busy_cond_;

std::set<TimerCallbackEntity> TimerPOSIX::cb_set_;
std::map<TimerCallback, TimerCallbackEntity> TimerPOSIX::cb_map_;
pthread_mutex_t TimerPOSIX::cb_lock_;

pthread_t TimerPOSIX::main_thread_;
pthread_t TimerPOSIX::worker_thread_;

TimerPOSIX::TimerPOSIX() { std::call_once(create_flag, TimerPOSIX::Create); }

/**
 * @brief Add callback to the timer.
 *
 * @param ms time interval to the deadline of callback, measured in millisecond
 * @param cb pointer of callback function
 * @param data argument passed to the callback
 *
 * @return true if success
 */
bool TimerPOSIX::AddCallback(long ms, const TimerCallback cb, void *data) {
    if (ms <= RES_POSIX_TIMER || nullptr == cb)
        return false;

    TimerCallbackEntity cbe(ms, cb, data);
    return AddCallback(cbe);
}

/**
 * @brief Add callback to the timer, it will enable the internal timer if needed.
 *
 * @param cbe callback entity
 *
 * @return true if success
 */
bool TimerPOSIX::AddCallback(const TimerCallbackEntity &cbe) {
    if (cbe.GetInterval() <= RES_POSIX_TIMER || nullptr == cbe.GetCallback())
        return false;

    pthread_mutex_lock(&cb_lock_);

    if (cb_map_.find(cbe.GetCallback()) != cb_map_.end()) {
        pthread_mutex_unlock(&cb_lock_);
        return false;
    }

    cb_set_.insert(cbe);
    cb_map_[cbe.GetCallback()] = cbe;

    if (timer_disabled_) {
        EnableTimer();
        printf("[%s] start timer\n", __func__);
    }

    pthread_mutex_unlock(&cb_lock_);

    return true;
}

/**
 * @brief Remove callback from the timer, it will disable internal timer if the callbacks set is empty.
 *
 * @param cb callback function that has been added before
 *
 * @return true if success
 */
bool TimerPOSIX::RemoveCallback(const TimerCallback cb) {
    if (nullptr == cb)
        return false;

    pthread_mutex_lock(&cb_lock_);

    auto itr = cb_map_.find(cb);
    if (cb_map_.end() == itr) {
        pthread_mutex_unlock(&cb_lock_);
        return false;
    }
    cb_set_.erase(itr->second);
    cb_map_.erase(itr);

    if (cb_set_.empty()) {
        DisableTimer();
        printf("[%s] stop timer\n", __func__);
    }
    pthread_mutex_unlock(&cb_lock_);
    return true;
}

/**
 * @brief Initialize TimerPOSIX.
 */
void TimerPOSIX::Create() {
    timer_disabled_ = true;
    struct sigevent ev = {
        .sigev_signo = SIG_POSIX_TIMER,
        .sigev_notify = SIGEV_SIGNAL,
    };
    assert(!timer_create(CLOCKID_TIMER, &ev, &posix_timerid_));

    assert(!sigemptyset(&sigset_));
    assert(!sigaddset(&sigset_, SIG_POSIX_TIMER));
    assert(!pthread_sigmask(SIG_BLOCK, &sigset_, NULL));

    worker_busy_ = false;
    assert(!pthread_mutex_init(&worker_busy_lock_, NULL));
    assert(!pthread_cond_init(&worker_busy_cond_, NULL));

    assert(!pthread_mutex_init(&cb_lock_, NULL));

    assert(!pthread_create(&main_thread_, NULL, TimerPOSIX::MainThreadEntry, NULL));
    assert(!pthread_create(&worker_thread_, NULL, TimerPOSIX::WorkerThreadEntry, NULL));
}

/**
 * @brief The main thread entry. It waits for the signal then notify the worker thread.
 */
void *TimerPOSIX::MainThreadEntry(void *data) {
    int err = 0;
    int sig = 0;
    while (true) {
        err = sigwait(&sigset_, &sig);
        assert(0 == err && SIG_POSIX_TIMER == sig);

        if (pthread_mutex_trylock(&worker_busy_lock_))
            continue;

        worker_busy_ = true;
        pthread_mutex_unlock(&worker_busy_lock_);
        pthread_cond_signal(&worker_busy_cond_);
        printf("[%s] notify!\n", __func__);
    }

    return NULL;
}

/**
 * @brief The worker thread entry. It waits for busy condition to become ture then handle the callbacks.
 */
void *TimerPOSIX::WorkerThreadEntry(void *data) {
    while (true) {
        pthread_mutex_lock(&worker_busy_lock_);
        while (!worker_busy_) {
            pthread_cond_wait(&worker_busy_cond_, &worker_busy_lock_);
        }

        pthread_mutex_lock(&cb_lock_);
        HandleCallbacks();

        if (cb_set_.empty()) {
            DisableTimer();
            printf("[%s] stop timer\n", __func__);
        }
        pthread_mutex_unlock(&cb_lock_);

        worker_busy_ = false;
        pthread_mutex_unlock(&worker_busy_lock_);
    }

    return NULL;
}

/**
 * @brief Enable internal POSIX timer.
 */
void TimerPOSIX::EnableTimer() {
    if (!timer_disabled_)
        return;

    struct itimerspec itv;
    clock_gettime(CLOCKID_TIMER, &itv.it_value);
    itv.it_value.tv_sec = (RES_POSIX_TIMER / 1000);
    itv.it_value.tv_nsec = (RES_POSIX_TIMER % 1000) * 1000000;
    itv.it_interval.tv_sec = (RES_POSIX_TIMER / 1000);
    itv.it_interval.tv_nsec = (RES_POSIX_TIMER % 1000) * 1000000;

    timer_settime(posix_timerid_, 0, &itv, NULL);
    timer_disabled_ = false;
}

/**
 * @brief Disable internal POSIX timer.
 */
void TimerPOSIX::DisableTimer() {
    struct itimerspec itv;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_nsec = 0;
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_nsec = 0;

    timer_settime(posix_timerid_, 0, &itv, NULL);
    timer_disabled_ = true;
}

/**
 * @brief Used by the worker thread, all the expired callbacks are called here.
 */
void TimerPOSIX::HandleCallbacks() {
    struct timespec now;
    TimerCallback cb = nullptr;
    void *data = nullptr;

    auto itr = cb_set_.begin();
    while (itr != cb_set_.end()) {
        clock_gettime(CLOCKID_TIMER, &now);
        if (LaterThan(&now, itr->GetDeadline())) {
            // TODO: maybe create a new thread to execute callback, so the buggy callback can never block timer
            cb = itr->GetCallback();
            data = itr->GetData();
            cb(data);

            cb_map_.erase(cb);
            itr = cb_set_.erase(itr);
        } else
            break;
    }
}

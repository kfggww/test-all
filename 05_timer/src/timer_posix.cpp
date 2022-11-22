#include "timer_posix.h"

#include <assert.h>

/*Normal timer resolution*/
#define TIMER_NORMAL_RESOLUTION 500

std::once_flag TimerPOSIX::create_flag_;

TimerPOSIX::TimerPOSIX() { std::call_once(TimerPOSIX::create_flag_, TimerPOSIX::Create); }

/**
 * @brief Get a real time signal number.
 *
 * @return -1 if no real time signal can be used, otherwise signal number
 */
int TimerPOSIX::GetRealTimeSignalNo() {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    static long sigs = 0;

    pthread_mutex_lock(&lock);
    int i = SIGRTMIN;
    while (i < SIGRTMAX) {
        if ((sigs & (1 << i)) == 0) {
            sigs |= (1 << i);
            break;
        }
        i++;
    }
    pthread_mutex_unlock(&lock);

    return i < SIGRTMAX ? i : -1;
}

void TimerPOSIX::Create() {
    sigset_t sigset;
    assert(0 == sigemptyset(&sigset));
    for (int i = SIGRTMIN; i < SIGRTMAX; ++i)
        assert(0 == sigaddset(&sigset, i));
    assert(0 == pthread_sigmask(SIG_BLOCK, &sigset, NULL));
}

TimerHighResolution::TimerHighResolution() : created_(false) {
    signo_ = TimerPOSIX::GetRealTimeSignalNo();
    if (signo_ != -1) {
        struct sigevent ev = {
            .sigev_signo = signo_,
            .sigev_notify = SIGEV_SIGNAL,
        };
        assert(0 == timer_create(CLOCKID_TIMER, &ev, &posix_timerid_));
        sigset_t sigset;
        assert(0 == sigemptyset(&sigset));
        assert(0 == sigaddset(&sigset, signo_));
        assert(0 == pthread_sigmask(SIG_BLOCK, &sigset, NULL));
        pthread_create(&worker_thread_, NULL, TimerHighResolution::WorkerThreadEntry, this);
        created_ = true;
    }
}

bool TimerHighResolution::AddCallback(const TimerCallbackEntity &cbe) {
    if (!created_ && !cbe.IsValid())
        return false;

    cb_entity_ = cbe;

    struct itimerspec itv;
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_nsec = 0;

    long interval_ms = cb_entity_.GetIntervalMs();
    long interval_ns = cb_entity_.GetIntervalNs();

    if (interval_ms != 0) {
        itv.it_value.tv_sec = (interval_ms / 1000);
        itv.it_value.tv_nsec = (interval_ms % 1000) * 1000000;
    } else if (interval_ns != 0) {
        itv.it_value.tv_sec = interval_ns / 1000000000;
        itv.it_value.tv_nsec = interval_ns % 1000000000;
    }

    assert(0 == timer_settime(posix_timerid_, 0, &itv, NULL));

    return true;
}

bool TimerHighResolution::RemoveCallback(const TimerCallback cb) { return true; }

void *TimerHighResolution::WorkerThreadEntry(void *data) {
    TimerHighResolution *tp = static_cast<TimerHighResolution *>(data);
    if (nullptr == tp)
        return NULL;

    int err = 0;
    int sig;
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, tp->signo_);

    while (true) {
        err = sigwait(&sigset, &sig);
        if (err || sig != tp->signo_ || !tp->cb_entity_.IsValid())
            continue;

        TimerCallback cb = tp->cb_entity_.GetCallback();
        void *cb_arg = tp->cb_entity_.GetData();
        cb(cb_arg);

        tp->cb_entity_.Reset();
    }

    return NULL;
}

std::once_flag TimerNormalResolution::create_flag_;

timer_t TimerNormalResolution::posix_timerid_;
sigset_t TimerNormalResolution::sigset_;
bool TimerNormalResolution::timer_disabled_;
int TimerNormalResolution::signo_;

volatile bool TimerNormalResolution::worker_busy_;
pthread_mutex_t TimerNormalResolution::worker_busy_lock_;
pthread_cond_t TimerNormalResolution::worker_busy_cond_;

std::set<TimerCallbackEntity> TimerNormalResolution::cb_set_;
std::map<TimerCallback, TimerCallbackEntity> TimerNormalResolution::cb_map_;
pthread_mutex_t TimerNormalResolution::cb_lock_;

pthread_t TimerNormalResolution::main_thread_;
pthread_t TimerNormalResolution::worker_thread_;

TimerNormalResolution::TimerNormalResolution() {
    std::call_once(TimerNormalResolution::create_flag_, TimerNormalResolution::Create);
}

/**
 * @brief Add callback to the timer, it will enable the internal timer if needed.
 *
 * @param cbe callback entity
 *
 * @return true if success
 */
bool TimerNormalResolution::AddCallback(const TimerCallbackEntity &cbe) {
    if (cbe.GetIntervalMs() <= TIMER_NORMAL_RESOLUTION || nullptr == cbe.GetCallback())
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
bool TimerNormalResolution::RemoveCallback(const TimerCallback cb) {
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
void TimerNormalResolution::Create() {
    timer_disabled_ = true;
    signo_ = TimerPOSIX::GetRealTimeSignalNo();

    struct sigevent ev = {
        .sigev_signo = signo_,
        .sigev_notify = SIGEV_SIGNAL,
    };
    assert(!timer_create(CLOCKID_TIMER, &ev, &posix_timerid_));

    assert(!sigemptyset(&sigset_));
    assert(!sigaddset(&sigset_, signo_));
    assert(!pthread_sigmask(SIG_BLOCK, &sigset_, NULL));

    worker_busy_ = false;
    assert(!pthread_mutex_init(&worker_busy_lock_, NULL));
    assert(!pthread_cond_init(&worker_busy_cond_, NULL));

    assert(!pthread_mutex_init(&cb_lock_, NULL));

    assert(!pthread_create(&main_thread_, NULL, TimerNormalResolution::MainThreadEntry, NULL));
    assert(!pthread_create(&worker_thread_, NULL, TimerNormalResolution::WorkerThreadEntry, NULL));
}

/**
 * @brief The main thread entry. It waits for the signal then notify the worker thread.
 */
void *TimerNormalResolution::MainThreadEntry(void *data) {
    int err = 0;
    int sig = 0;
    while (true) {
        err = sigwait(&sigset_, &sig);
        if (err != 0 || sig != signo_)
            continue;

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
void *TimerNormalResolution::WorkerThreadEntry(void *data) {
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
void TimerNormalResolution::EnableTimer() {
    if (!timer_disabled_)
        return;

    struct itimerspec itv;
    itv.it_value.tv_sec = (TIMER_NORMAL_RESOLUTION / 1000);
    itv.it_value.tv_nsec = (TIMER_NORMAL_RESOLUTION % 1000) * 1000000;
    itv.it_interval.tv_sec = (TIMER_NORMAL_RESOLUTION / 1000);
    itv.it_interval.tv_nsec = (TIMER_NORMAL_RESOLUTION % 1000) * 1000000;

    timer_settime(posix_timerid_, 0, &itv, NULL);
    timer_disabled_ = false;
}

/**
 * @brief Disable internal POSIX timer.
 */
void TimerNormalResolution::DisableTimer() {
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
void TimerNormalResolution::HandleCallbacks() {
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

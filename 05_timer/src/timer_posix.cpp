#include "timer_posix.h"

#include <assert.h>
#include <cstring>

/*Normal timer resolution*/
#define TIMER_NORMAL_RESOLUTION 500

std::once_flag TimerPOSIX::create_flag_;
pthread_mutex_t TimerPOSIX::signo_bits_lock_;
std::bitset<128> TimerPOSIX::signo_bits_;

TimerPOSIX::TimerPOSIX() {
    std::call_once(TimerPOSIX::create_flag_, TimerPOSIX::Create);
}

/**
 * @brief Get a real time signal number.
 *
 * @return -1 if no real time signal can be used, signal number otherwise
 */
int TimerPOSIX::GetRealTimeSignalNo() {
    pthread_mutex_lock(&signo_bits_lock_);
    int i = SIGRTMIN;
    while (i <= SIGRTMAX) {
        if (!signo_bits_[i]) {
            signo_bits_.set(i);
            break;
        }
        i++;
    }
    pthread_mutex_unlock(&signo_bits_lock_);

    return i < SIGRTMAX ? i : -1;
}

/**
 * @brief Mark signal with number signo as unused.
 *
 * @param: signo
 */
void TimerPOSIX::PutRealTimeSignalNo(int signo) {
    if (signo < SIGRTMIN || signo > SIGRTMAX)
        return;
    pthread_mutex_lock(&signo_bits_lock_);
    signo_bits_.set(signo, false);
    pthread_mutex_unlock(&signo_bits_lock_);
}

/**
 * @brief TimerPOSIX class initialization.
 */
void TimerPOSIX::Create() {
    /*Mark all signo as unused.*/
    pthread_mutex_init(&signo_bits_lock_, NULL);
    signo_bits_.reset();

    /*Block all the real time signals*/
    sigset_t sigset;
    assert(!sigemptyset(&sigset));
    for (int i = SIGRTMIN; i <= SIGRTMAX; ++i)
        assert(!sigaddset(&sigset, i));
    sigaddset(&sigset, SIGQUIT);
    assert(!pthread_sigmask(SIG_BLOCK, &sigset, NULL));
}

/**
 * @brief Construct high resolution timer. Each object of this class has its own
 * real time signal number, posix timer id and worker thread. When some callback
 * is added, timer will be enabled, and a signal is sent to the worker thread if
 * the timer is expired.
 */
TimerHighResolution::TimerHighResolution() : created_(false) {
    pthread_mutex_init(&cb_entity_lock_, NULL);
    signo_ = TimerPOSIX::GetRealTimeSignalNo();
    if (signo_ != -1) {
        struct sigevent ev = {
            .sigev_signo = signo_,
            .sigev_notify = SIGEV_SIGNAL,
        };
        assert(!timer_create(CLOCK_MONOTONIC, &ev, &posix_timerid_));
        pthread_create(&worker_thread_, NULL,
                       TimerHighResolution::WorkerThreadEntry, this);
        created_ = true;
    }
}

/**
 * @brief Destruct high resolution timer. The real time signal number is
 * released, posix timer and worker thread associated with this object is freed.
 */
TimerHighResolution::~TimerHighResolution() {
    TimerPOSIX::PutRealTimeSignalNo(signo_);
    timer_delete(posix_timerid_);
    assert(!pthread_kill(worker_thread_, SIGQUIT));
}

/**
 * @brief Add a callback entity to the high resolution timer.
 *
 * @param cbe: callback entity
 *
 * @return false if no posix timer has been created, or "cbe" is invalid, or
 * there is already a valid callback entity inside current timer.
 */
bool TimerHighResolution::AddCallback(const TimerCallbackEntity &cbe) {
    pthread_mutex_lock(&cb_entity_lock_);

    if (!created_ || !cbe.IsValid() || cb_entity_.IsValid()) {
        pthread_mutex_unlock(&cb_entity_lock_);
        return false;
    }

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

    assert(!timer_settime(posix_timerid_, 0, &itv, NULL));
    pthread_mutex_unlock(&cb_entity_lock_);

    return true;
}

/**
 * @brief Remove callback from the high resolution timer.
 *
 * @param: cb: callback function that has been added before
 *
 * @return true if success
 */
bool TimerHighResolution::RemoveCallback(const TimerCallback cb) {
    if (!created_)
        return false;

    pthread_mutex_lock(&cb_entity_lock_);
    if (cb != cb_entity_.GetCallback()) {
        pthread_mutex_unlock(&cb_entity_lock_);
        return false;
    }
    cb_entity_.Reset();
    pthread_mutex_unlock(&cb_entity_lock_);

    return true;
}

/**
 * @brief Worker thread entry of high resolution timer. It handles callback if
 * signo_ arrives, or the thread exits if SIGQUIT is received.
 */
void *TimerHighResolution::WorkerThreadEntry(void *data) {
    TimerHighResolution *tp = static_cast<TimerHighResolution *>(data);
    if (nullptr == tp)
        return NULL;

    int err = 0;
    int sig;
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, tp->signo_);

    while (true) {
        err = sigwait(&sigset, &sig);
        if (err)
            continue;
        if (sig == SIGQUIT)
            break;

        pthread_mutex_lock(&tp->cb_entity_lock_);
        if (tp->cb_entity_.IsValid()) {
            TimerCallback cb = tp->cb_entity_.GetCallback();
            void *cb_arg = tp->cb_entity_.GetData();
            cb(cb_arg);
            tp->cb_entity_.Reset();
        }
        pthread_mutex_unlock(&tp->cb_entity_lock_);
    }

    return NULL;
}

std::once_flag TimerNormalResolution::create_flag_;

timer_t TimerNormalResolution::posix_timerid_;
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
    std::call_once(TimerNormalResolution::create_flag_,
                   TimerNormalResolution::Create);
}

/**
 * @brief Add callback to the normal resolution timer. It will enable the
 * internal timer if needed.
 *
 * @param cbe: callback entity
 *
 * @return true if success
 */
bool TimerNormalResolution::AddCallback(const TimerCallbackEntity &cbe) {
    if (cbe.GetIntervalMs() <= TIMER_NORMAL_RESOLUTION ||
        nullptr == cbe.GetCallback())
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
 * @brief Remove callback from the normal resolution timer. It will disable
 * internal timer if the callbacks set is empty.
 *
 * @param cb: callback function that has been added before
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
 * @brief TimerNormalResolution class initialization.
 */
void TimerNormalResolution::Create() {
    timer_disabled_ = true;
    signo_ = TimerPOSIX::GetRealTimeSignalNo();

    struct sigevent ev = {
        .sigev_signo = signo_,
        .sigev_notify = SIGEV_SIGNAL,
    };
    assert(!timer_create(CLOCK_MONOTONIC, &ev, &posix_timerid_));

    worker_busy_ = false;
    assert(!pthread_mutex_init(&worker_busy_lock_, NULL));
    assert(!pthread_cond_init(&worker_busy_cond_, NULL));

    assert(!pthread_mutex_init(&cb_lock_, NULL));

    assert(!pthread_create(&main_thread_, NULL,
                           TimerNormalResolution::MainThreadEntry, NULL));
    assert(!pthread_create(&worker_thread_, NULL,
                           TimerNormalResolution::WorkerThreadEntry, NULL));
}

/**
 * @brief The main thread entry of normal resolution timer. It waits for the
 * signal then notify the worker thread.
 */
void *TimerNormalResolution::MainThreadEntry(void *data) {
    int err = 0;
    int sig = 0;

    sigset_t sigset;
    assert(!sigemptyset(&sigset));
    assert(!sigaddset(&sigset, signo_));

    while (true) {
        err = sigwait(&sigset, &sig);
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
 * @brief The worker thread entry of normal resolution timer. It waits for busy
 * condition to become ture then handle the callbacks.
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
 * @brief Enable internal posix timer.
 */
void TimerNormalResolution::EnableTimer() {
    if (!timer_disabled_)
        return;

    struct itimerspec itv;
    itv.it_value.tv_sec = (TIMER_NORMAL_RESOLUTION / 1000);
    itv.it_value.tv_nsec = (TIMER_NORMAL_RESOLUTION % 1000) * 1000000;
    itv.it_interval.tv_sec = (TIMER_NORMAL_RESOLUTION / 1000);
    itv.it_interval.tv_nsec = (TIMER_NORMAL_RESOLUTION % 1000) * 1000000;

    assert(!timer_settime(posix_timerid_, 0, &itv, NULL));
    timer_disabled_ = false;
}

/**
 * @brief Disable internal posix timer.
 */
void TimerNormalResolution::DisableTimer() {
    if (timer_disabled_)
        return;

    struct itimerspec itv;
    memset(&itv, 0, sizeof(struct itimerspec));

    assert(!timer_settime(posix_timerid_, 0, &itv, NULL));
    timer_disabled_ = true;
}

/**
 * @brief Used by the worker thread of normal resolution timer, all the expired
 * callbacks are called here.
 */
void TimerNormalResolution::HandleCallbacks() {
    struct timespec now;
    TimerCallback cb = nullptr;
    void *data = nullptr;

    auto itr = cb_set_.begin();
    while (itr != cb_set_.end()) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (LaterThan(&now, itr->GetDeadline())) {
            // TODO: maybe create a new thread to execute callback, so the buggy
            // callback can never block timer
            cb = itr->GetCallback();
            data = itr->GetData();
            cb(data);

            cb_map_.erase(cb);
            itr = cb_set_.erase(itr);
        } else
            break;
    }
}

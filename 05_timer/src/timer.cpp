#include <time.h>

#include "timer.h"

bool LaterThan(const struct timespec *now, const struct timespec *deadline) {
    if (nullptr == now || nullptr == deadline)
        return false;

    if (now->tv_sec > deadline->tv_sec)
        return true;
    else if (now->tv_sec == deadline->tv_sec && now->tv_nsec >= deadline->tv_nsec)
        return true;
    else
        return false;
}

TimerCallbackEntity::TimerCallbackEntity(const TimerCallback cb, void *data, const long interval_ms,
                                         const long interval_ns)
    : callback_(cb), data_(data), interval_ms_(interval_ms), interval_ns_(interval_ns) {
    clock_gettime(CLOCKID_TIMER, &deadline_);
    if (interval_ms != 0) {
        deadline_.tv_sec += interval_ms / 1000;
        deadline_.tv_nsec += (interval_ms % 1000) * 1000000;
    } else if (interval_ns != 0) {
        deadline_.tv_sec += interval_ns / 1000000000;
        deadline_.tv_nsec += (interval_ns % 1000000000);
    }
}

void TimerCallbackEntity::Reset() {
    interval_ms_ = 0;
    interval_ns_ = 0;
    callback_ = nullptr;
    data_ = nullptr;
}

bool TimerCallbackEntity::IsValid() const {
    if (interval_ms_ < 0 || interval_ns_ < 0 || callback_ == nullptr || (interval_ms_ == 0 && interval_ns_ == 0))
        return false;
    return true;
}

bool TimerCallbackEntity::operator<(const TimerCallbackEntity &other) const {
    return !LaterThan(&deadline_, &other.deadline_);
}

const long TimerCallbackEntity::GetIntervalMs() const { return interval_ms_; }

const long TimerCallbackEntity::GetIntervalNs() const { return interval_ns_; }

const struct timespec *TimerCallbackEntity::GetDeadline() const { return &deadline_; }

TimerCallback TimerCallbackEntity::GetCallback() const { return callback_; }

void *TimerCallbackEntity::GetData() const { return data_; }
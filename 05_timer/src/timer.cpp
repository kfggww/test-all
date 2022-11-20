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

TimerCallbackEntity::TimerCallbackEntity(const long interval_ms, const TimerCallback cb, void *data)
    : interval_ms_(interval_ms), callback_(cb), data_(data) {
    clock_gettime(CLOCKID_TIMER, &deadline_);
    deadline_.tv_sec += interval_ms / 1000;
    deadline_.tv_nsec += (interval_ms % 1000) * 1000000;
}

bool TimerCallbackEntity::operator<(const TimerCallbackEntity &other) const { return !LaterThan(&deadline_, &other.deadline_); }

const long TimerCallbackEntity::GetInterval() const { return interval_ms_; }

const struct timespec *TimerCallbackEntity::GetDeadline() const { return &deadline_; }

TimerCallback TimerCallbackEntity::GetCallback() const { return callback_; }

void *TimerCallbackEntity::GetData() const { return data_; }
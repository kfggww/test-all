#include <time.h>

#include "timer.h"

/**
 * @brief if the absolute time "now" is later than the "deadline".
 *
 * @param now
 * @param deadline
 *
 * @return true if "now" is later than "deadline"
 */
bool LaterThan(const struct timespec *now, const struct timespec *deadline) {
    if (nullptr == now || nullptr == deadline)
        return false;

    if (now->tv_sec > deadline->tv_sec)
        return true;
    else if (now->tv_sec == deadline->tv_sec &&
             now->tv_nsec >= deadline->tv_nsec)
        return true;
    else
        return false;
}

/**
 * @brief Construct callback entity. The callback should be called after
 * interval_ms milliseconds or interval_ns nanoseconds.
 *
 * @param cb: callback function pointer
 * @param data: callback function argument
 * @param interval_ms: time interval measured in milliseconds
 * @param interval_ns: timer interval measured in nanoseconds
 *
 * @note NEVER set both the interval_ms and interval_ns to non-zero value
 */
TimerCallbackEntity::TimerCallbackEntity(const TimerCallback cb, void *data,
                                         const long interval_ms,
                                         const long interval_ns)
    : callback_(cb), data_(data), interval_ms_(interval_ms),
      interval_ns_(interval_ns) {
    clock_gettime(CLOCK_MONOTONIC, &deadline_);
    if (interval_ms != 0) {
        deadline_.tv_sec += interval_ms / 1000;
        deadline_.tv_nsec += (interval_ms % 1000) * 1000000;
    } else if (interval_ns != 0) {
        deadline_.tv_sec += interval_ns / 1000000000;
        deadline_.tv_nsec += (interval_ns % 1000000000);
    }
}

/**
 * @brief Reset the callback entity to invalid state.
 */
void TimerCallbackEntity::Reset() {
    interval_ms_ = 0;
    interval_ns_ = 0;
    callback_ = nullptr;
    data_ = nullptr;
}

/**
 * @brief If the callback entity is valid.
 *
 * @return true if valid
 */
bool TimerCallbackEntity::IsValid() const {
    if (interval_ms_ < 0 || interval_ns_ < 0 || callback_ == nullptr ||
        (interval_ms_ == 0 && interval_ns_ == 0))
        return false;
    return true;
}

/**
 * @brief If the deadline point is smaller than the other's.
 */
bool TimerCallbackEntity::operator<(const TimerCallbackEntity &other) const {
    return !LaterThan(&deadline_, &other.deadline_);
}

/**
 * @brief Get callback function interval_ms.
 */
const long TimerCallbackEntity::GetIntervalMs() const { return interval_ms_; }

/**
 * @brief Get callback function interval_ns.
 */
const long TimerCallbackEntity::GetIntervalNs() const { return interval_ns_; }

/**
 * @brief Get callback function deadline.
 */
const struct timespec *TimerCallbackEntity::GetDeadline() const {
    return &deadline_;
}

/**
 * @brief Get callback function pointer.
 */
TimerCallback TimerCallbackEntity::GetCallback() const { return callback_; }

/**
 * @brief Get callback function argument.
 */
void *TimerCallbackEntity::GetData() const { return data_; }
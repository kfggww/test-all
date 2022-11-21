#ifndef _TIMER_H_
#define _TIMER_H_

#define CLOCKID_TIMER CLOCK_MONOTONIC

typedef void (*TimerCallback)(void *);
class TimerCallbackEntity;

/**
 * @brief The top level class, declare what kind of services it provides.
 */
class Timer {
  public:
    Timer() = default;
    virtual ~Timer() = default;
    virtual bool AddCallback(const TimerCallbackEntity &cbe) = 0;
    virtual bool RemoveCallback(const TimerCallback cb) = 0;
};

/**
 * @brief Designed to be used by the client programs, they pass all the necessary informations to the timer through the
 * object of this class.
 *
 */
class TimerCallbackEntity {
  public:
    TimerCallbackEntity() = default;
    TimerCallbackEntity(const TimerCallbackEntity &other) = default;
    TimerCallbackEntity(const TimerCallback cb, void *data, const long interval_ms, const long interval_ns = 0);

    void Reset();
    bool IsValid() const;
    bool operator<(const TimerCallbackEntity &other) const;

    const long GetIntervalMs() const;
    const long GetIntervalNs() const;
    const struct timespec *GetDeadline() const;
    TimerCallback GetCallback() const;
    void *GetData() const;

  private:
    TimerCallback callback_;
    void *data_;

    long interval_ms_;
    long interval_ns_;
    struct timespec deadline_;
};

/**
 * @brief if the absolute time "now" is later than the "deadline".
 *
 * @param now
 * @param deadline
 *
 * @return true if "now" is later than "deadline"
 */
bool LaterThan(const struct timespec *now, const struct timespec *deadline);

#endif
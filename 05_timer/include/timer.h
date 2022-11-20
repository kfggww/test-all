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
    virtual bool AddCallback(long ms, const TimerCallback cb, void *data) = 0;
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
    TimerCallbackEntity(const long interval_ms, const TimerCallback cb, void *data);

    bool operator<(const TimerCallbackEntity &other) const;

    const long GetInterval() const;
    const struct timespec *GetDeadline() const;
    TimerCallback GetCallback() const;
    void *GetData() const;

  private:
    long interval_ms_;
    struct timespec deadline_;
    TimerCallback callback_;
    void *data_;
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
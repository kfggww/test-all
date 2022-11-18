#ifndef _TIMER_H_
#define _TIMER_H_

typedef void (*TimerCallback)(void *);

class Timer {
   public:
    Timer() = default;
    virtual ~Timer() = default;
    virtual bool AddCallback(long ms, const TimerCallback cb, void *data) = 0;
    virtual bool RemoveCallback(const TimerCallback cb) = 0;
};

#endif
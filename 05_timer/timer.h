#ifndef _TIMER_H_
#define _TIMER_H_

typedef void (*TimerCallback)(void *);

class Timer {
  public:
    Timer() {}
    virtual ~Timer() {}
    virtual bool AddCallback(long ms, const TimerCallback cb, void *data) { return false; }
    virtual bool RemoveCallback(const TimerCallback cb) { return false; }
};

#endif
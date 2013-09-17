#ifndef __CLOCK_H__
#define __CLOCK_H__

extern  void initClock(void (*handler)(int));
extern  void uninitClock(void);
extern  void lock(void);
extern  void unlock(void);

#endif

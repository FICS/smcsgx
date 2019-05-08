#ifndef _TIMING_H
#define _TIMING_H

#ifdef SGX_TIMING_ENABLED

void timing_start();
void timing_tick(const char * tag, const char * desc, ...);
void timing_stop();

#else

#define timing_start()
#define timing_tick(tag, desc, ...)
#define timing_stop()

#endif

#endif

#include "../include/timing.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef SGX_TIMING_ENABLED

// XXX: we only support one global timer
static struct timeval tv_start, tv_end, delta;
static bool started = false;

void timing_start()
{
  gettimeofday(&tv_start, NULL);
  started = true;
}

void timing_tick(const char * tag, const char * desc, ...)
{
  if(!started)
    return;

  gettimeofday(&tv_end, NULL);
  timersub(&tv_end, &tv_start, &delta);

  if(desc && desc[0]) {
    va_list va;
    va_start(va, desc);

    printf("TIMER: [%s] %ld.%06ld (", tag, delta.tv_sec, delta.tv_usec);

    vprintf(desc, va);

    printf(")\n");

    va_end(va);
  }
  else
    printf("TIMER: [%s] %ld.%06ld\n", tag, delta.tv_sec, delta.tv_usec);
}

void timing_stop()
{
  gettimeofday(&tv_end, NULL);
  timersub(&tv_end, &tv_start, &delta);
  printf("TIMER END: %ld.%06ld\n", delta.tv_sec, delta.tv_usec);
  started = false;
}

#endif

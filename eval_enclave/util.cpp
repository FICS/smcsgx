#include <stdarg.h>
#include <stdio.h>

#include "eval_enclave_t.h"

int eprintf(const char * fmt, ...)
{
    va_list ap;
    char buf[BUFSIZ] = {0};

    va_start(ap, fmt);
    int amt = vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);

    debug_string(buf);

    return amt;
}

static inline void log_trace(const char * tag, void * address, const char * type, size_t size)
{
  if(size)
    eprintf("MEM_TRACE %s: %s 0x%016lx %zu", tag, type, address, size);
  else
    eprintf("MEM_TRACE %s: %s 0x%016lx", tag, type, address);
}

void trace_mem_write(const char * tag, void * address)
{
  log_trace(tag, address, "W", 0);
}

void trace_mem_read(const char * tag, void * address)
{
  log_trace(tag, address, "R", 0);
}

void trace_mem_rw(const char * tag, void * address)
{
  log_trace(tag, address, "A", 0);
}

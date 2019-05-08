#ifndef UTIL_H_
#define UTIL_H_

int eprintf(const char * fmt, ...);
void trace_mem_write(const char * tag, void * address);
void trace_mem_read(const char * tag, void * address);
void trace_mem_rw(const char * tag, void * address);

#endif

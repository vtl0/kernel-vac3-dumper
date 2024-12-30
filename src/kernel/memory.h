#ifndef MEMORY_H_
#define MEMORY_H_

#include <linux/types.h>

ssize_t mem_rw(pid_t pid, char *buf, size_t count, unsigned long addr,
               int write);

#endif  // MEMORY_H_

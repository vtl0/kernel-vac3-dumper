#ifndef LOGGING_H_
#define LOGGING_H_

#include <stdio.h>

#define LOG_ENTRY "[\e[1;38;5;40m>\e[0m] "
#define LOG_INFO "[\e[1;38;5;39mi\e[0m] "
#define LOG_WARN "[\e[1;38;5;11m!\e[0m] "
#define LOG_ERROR "[\e[1;38;5;196mX\e[0m] "

int fprintlog(FILE *f, const char *fmt, ...);
int printlog(const char *fmt, ...);

#define PRINT_WHERE(f) \
  fprintlog(f, "At line " __LINE__ " in file \"" __FILE__ "\"\n");

#endif  // LOGGING_H_

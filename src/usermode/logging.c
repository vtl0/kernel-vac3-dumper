#include "logging.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *remove_ansi_esc(const char *str) {
  unsigned long long size, i, j;
  int is_ansi_esc;
  char *new_str;

  size = strlen(str);
  is_ansi_esc = 0;

  new_str = malloc(size + 1);

  if (NULL == new_str) {
    return NULL;
  }

  for (i = 0, j = 0; i < size; i++) {
    if ('\x1B' == str[i] || '\x9B' == str[i]) {
      is_ansi_esc = 1;
      continue;
    }

    if (0 == is_ansi_esc) {
      goto _loop_end;
    }

    if (1 == is_ansi_esc && '[' == str[i]) {
      is_ansi_esc++;
      continue;
    }

    if (2 == is_ansi_esc) {
      if (isdigit(str[i]) || isspace(str[i])) {
        continue;
      }

      if (';' == str[i]) {
        continue;
      }

      is_ansi_esc = 0;
      continue;
    }

  _loop_end:

    new_str[j] = str[i];
    j++;
  }

  new_str[j] = '\0';

  return new_str;
}

int fprintlog(FILE *stream, const char *fmt, ...) {
  int return_value;
  va_list args;
  char *str;

#if defined(_NO_LOG)
  return 0;
#endif

  va_start(args, fmt);

  if (0 != isatty(fileno(stream))) {
    return_value = vfprintf(stream, fmt, args);

    va_end(args);

    return return_value;
  }

  str = remove_ansi_esc(fmt);

  if (NULL == str) {
    return -1;
  }

  return_value = vfprintf(stream, str, args);

  free(str);
  va_end(args);

  return return_value;
}

int printlog(const char *fmt, ...) {
  int return_value;
  va_list args;
  char *str;

#if defined(_NO_LOG)
  return 0;
#endif

  va_start(args, fmt);

  if (0 != isatty(STDOUT_FILENO)) {
    return_value = vprintf(fmt, args);

    va_end(args);

    return return_value;
  }

  str = remove_ansi_esc(fmt);

  if (NULL == str) {
    return -1;
  }

  return_value = vprintf(str, args);

  free(str);
  va_end(args);

  return return_value;
}

#include "controller.h"

#include <sys/types.h>
#include <unistd.h>

ssize_t echo_mod(void) {
  register unsigned long _key asm("r14") = VAC3DUMP;
  register unsigned long cmd asm("r15") = CMD_ECHO;
  unsigned long _rax;

  asm volatile("syscall"
               : "=a"(_rax)
               : "a"(9), "r"(_key), "r"(cmd)
               : "rcx", "r11", "memory");

  if (VAC3DUMP != _rax) return 0;

  return 1;
}

ssize_t register_vac_dumper(void *buf, ssize_t len) {
  register void *_dst asm("rdi") = buf;
  register unsigned long _len asm("rsi") = len;
  register unsigned long _pid asm("rdx") = getpid();
  register unsigned long key asm("r14") = VAC3DUMP;
  register unsigned long cmd asm("r15") = CMD_REGISTERBUF;
  unsigned long _rax;

  asm volatile("syscall"
               : "=a"(_rax)
               : "a"(9), "r"(_pid), "r"(_len), "r"(_dst), "r"(key), "r"(cmd)
               : "rcx", "r11", "memory");

  return _rax;
}

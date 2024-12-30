#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <sys/types.h>

#define CMD_ECHO 0
#define CMD_REGISTERBUF 1

#define VAC3DUMP 0x504d554433434156

ssize_t echo_mod(void);
ssize_t register_vac_dumper(void *dst, ssize_t len);

#endif  // CONTROLLER_H_

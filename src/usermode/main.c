#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#include "controller.h"
#include "logging.h"
#include "memory.h"

struct vac_module {
  uint32_t size;
  uint8_t data_offset;
};

static volatile sig_atomic_t should_exit = 0;

static void sig_handler(int sig) {
  switch (sig) {
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
      should_exit = 1;
      break;
    default:
      break;
  }
}

static void recv_mod_loop(struct vac_module *dst, size_t len) {
  struct tm *timeinfo;
  time_t rawtime;
  uint32_t crc;

  memset(dst, 0, len);
  while (!should_exit) {
    usleep(1000);
    if (0 != dst->size) {
      char path[32];
      int fd;

      crc = crc32(0x80000000, &dst->data_offset, dst->size);
      time(&rawtime);
      timeinfo = localtime(&rawtime);
      printlog(LOG_ENTRY
               "VAC has loaded module 0x%x at "
               "%s",
               crc, asctime(timeinfo));
      sprintf(path, "%x.so", crc);
      fd = open(path, O_RDWR | O_CREAT, 0600);
      if (0 > fd) {
        if (EEXIST != errno) {
          fprintlog(stderr,
                    LOG_ERROR
                    "Failed to create file"
                    " \"%s\" (\"%s\")\n",
                    path, strerror(errno));
        }
        memset(dst, 0, len);
        continue;
      }
      if (dst->size != write(fd, &dst->data_offset, dst->size)) {
        fprintlog(stderr,
                  LOG_ERROR
                  "Failed to write module to"
                  " \"%s\" (\"%s\")\n",
                  path, strerror(errno));
      } else
        fprintlog(stderr,
                  LOG_ENTRY
                  "Dumped VAC module to"
                  " \"%s\"\n",
                  path);
      close(fd);
      memset(dst, 0, len);
    }
  }
}

int main(void) {
  struct vac_module *dst;
  size_t len;
  struct sigaction action;
  sigset_t set;

  printlog(LOG_ENTRY "Starting vac3_dumper\n");
  if (!echo_mod()) {
    fprintlog(stderr, LOG_ERROR "Kernel module is not loaded\n");
    return EXIT_FAILURE;
  }

  memset(&action, 0, sizeof(action));
  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGQUIT);
  action.sa_mask = set;
  action.sa_handler = sig_handler;
  action.sa_flags = SA_RESTART;
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGQUIT, &action, NULL);

  len = 52428800 + 4;
  dst = malloc(len);
  if (NULL == dst) {
    fprintlog(stderr,
              LOG_ERROR
              "Failed to allocate %lu bytes "
              "(\"%s\")\n",
              len, strerror(errno));
    return EXIT_FAILURE;
  }

  if (VAC3DUMP != register_vac_dumper(dst, len)) {
    fprintlog(stderr, LOG_ERROR "Failed to register dumper\n");
    free(dst);
    return EXIT_FAILURE;
  }

  printlog(LOG_ENTRY "Registered buf at %p with len %lu\n", dst, len);

  // main loop
  recv_mod_loop(dst, len);

  free(dst);
  return EXIT_SUCCESS;
}

#include "hooks.h"

#include <linux/kprobes.h>
#include <linux/types.h>

#include "memory.h"

static unsigned int localvar_offset;
static pid_t receiver_pid;
static unsigned long receiver_buf;
static size_t receiver_size;
static size_t receiver_real_size;

static int dump_module(void *struct_ptr) {
  uint32_t mod_size;
  uint32_t mod_buf;
  void *size_ptr;
  unsigned long user_data_ptr;
  unsigned long ret;
  ssize_t res;
  uint8_t module_copy[4];

  if (0 == receiver_pid || 0 == receiver_buf || 4 >= receiver_size ||
      0 == receiver_real_size || (void *)4 > struct_ptr)
    return 0;

  if (0 != copy_from_user(&mod_buf, struct_ptr, sizeof(mod_buf))) {
    printk(KERN_ERR
           "[vac3_dumper] Could not copy module pointer "
           "from %p\n",
           struct_ptr);
    return -1;
  }
  size_ptr = (void *)((unsigned long)struct_ptr - sizeof(mod_buf));
  if (0 != copy_from_user(&mod_size, size_ptr, sizeof(mod_size))) {
    printk(KERN_ERR
           "[vac3_dumper] Could not copy module size "
           "from %p\n",
           size_ptr);
    return -1;
  }

  if (0 == mod_buf) {
    printk(KERN_ERR
           "[vac3_dumper] Module pointer is NULL, "
           "Ignoring...\n");
    return -1;
  }

  // ensure the module is between 128 bytes and mod_size
  // long. the smallest possible valid standalone hello
  // world in linux is around 130 bytes
  if (128 > mod_size || receiver_real_size < mod_size) {
    printk(KERN_ERR
           "[vac3_dumper] Module size is abnormal, at "
           "%u bytes long. Ignoring...\n",
           mod_size);
    return -1;
  }

  // Generally speaking, it is a terrible idea to do this.
  // Ideally you'd communicate through a /proc/ FIFO to stream
  // data to usermode. For now, this dumper only supports one
  // registered usermode buffer to store VAC module data, with
  // no signaling to the process, requiring the structure to be
  // monitored in real time for changes. TODO: make this better

  ret = copy_from_user(module_copy, (void *)(unsigned long)mod_buf,
                       sizeof(module_copy));
  if (0 != ret) {
    printk(KERN_ERR
           "[vac3_dumper] Could not copy the module as "
           "%lu bytes failed to be read\n",
           ret);
    return -1;
  }

  if (0 != memcmp("\x7f\x45\x4c\x46", module_copy, 4)) {
    printk(KERN_WARNING
           "[vac3_dumper] The data in %p does not look "
           "like a module as the magic number does not"
           " match \\x7FELF, ignoring...\n",
           (void *)(unsigned long)mod_buf);
    return -1;
  }

  res = mem_rw(receiver_pid, size_ptr, sizeof(mod_size), receiver_buf, 1);
  if (0 >= res) {
    printk(KERN_ERR "[vac3_dumper] Failed to write VAC dump\n");
    return -1;
  }
  user_data_ptr = receiver_buf + sizeof(mod_size);
  res = mem_rw(receiver_pid, (void *)(unsigned long)mod_buf, (size_t)mod_size,
               user_data_ptr, 1);
  if (0 >= res) {
    printk(KERN_ERR "[vac3_dumper] Failed to write VAC dump\n");
    return -1;
  }

  return 0;
}

int install_hooks(struct kprobe *list) {
  int i, j;

  // loop until a invalid entry
  for (i = 0; list[i].symbol_name != NULL || list[i].addr != NULL; i++) {
    // not a valid hook, ignore
    if (NULL == list[i].pre_handler && NULL == list[i].post_handler) continue;

    if (0 > register_kprobe(&list[i])) {
      if (NULL != list[i].symbol_name) {
        printk(KERN_ERR "[vac3_dumper] Failed to hook %s\n",
               list[i].symbol_name);
      } else {
        printk(KERN_ERR "[vac3_dumper] Failed to hook %p\n", list[i].addr);
      }
      // unregister all registered before returning
      for (j = 0; j < i; j++) unregister_kprobe(&list[j]);

      return -1;
    }
  }

  return 0;
}

// huge thanks for h33p's vaclog project. absolute legend.
// https://github.com/h33p/vaclog.git
int ia32_mmap_hk(struct kprobe *probe, struct pt_regs *regs) {
  struct pt_regs *usermode_regs = (struct pt_regs *)regs->di;
  uint8_t *struct_ptr;

  // we are looking for ClientModuleManager thread
  if (0 != strcmp(current->comm, "ClientModuleMan")) return 0;

  if (0 == localvar_offset) {
    uint8_t stack_copy[1024];
    size_t size;
    unsigned long ret;
    int i;
    uint8_t expected_magic[4];

    /* We begin at the mmap stack frame. Its size depends on various
     * factors such as custom libc compilation/flavor or version. So
     * while we could hardcode a offset for its stack frame size, it
     * is ideal to dynamically identify structures for compatibility
     * with varying linux distributions, differing VAC3 versions and
     * every potential environmental factor which could move offsets
     * by a unpredictable margin.
     *
     * This could be a bad idea, although I can't see how it'd break;
     * With that said, may fire walk with me (as I walk the stack)
     */
    memset(stack_copy, 0, sizeof(stack_copy));
    ret = copy_from_user(stack_copy, (void *)usermode_regs->sp,
                         sizeof(stack_copy));
    if (0 != ret) {
      printk(KERN_ERR
             "[vac3_dumper] Not all bytes from stack could be copied,"
             " as %lu failed to be read from\n",
             ret);
      return 0;
    }

    for (i = 4; i < sizeof(stack_copy); i++) {
      uint64_t _current_32b_ptr = *(uint32_t *)&stack_copy[i];
      void *current_32b_ptr = (void *)_current_32b_ptr;

      if (!access_ok(current_32b_ptr, 4)) continue;

      if (0 != copy_from_user(expected_magic, current_32b_ptr,
                              sizeof(expected_magic)))
        continue;

      // if is \x7FELF, then we found
      if (0 != memcmp("\x7f\x45\x4c\x46", expected_magic, 4)) continue;

      size = *((uint32_t *)&stack_copy[i - 4]);

      if (128 > size || 262144 < size) {
        printk(KERN_INFO
               "[vac3_dumper] skipping "
               "offset 0x%x which contains size %lu\n",
               i, size);
        continue;
      }

      localvar_offset = i;
      printk(KERN_NOTICE
             "[vac3_dumper] Found localvar_offset "
             "through memory footprinting. It is "
             "located at offset 0x%x; size %lu\n",
             localvar_offset, size);
      break;
    }
    if (0 == localvar_offset) return 0;
  }

  struct_ptr = (uint8_t *)usermode_regs->sp;
  struct_ptr += localvar_offset;

  if (0 > dump_module(struct_ptr)) return 0;

  return 0;
}

void uninstall_hooks(struct kprobe *list) {
  int i;
  // loop until a invalid entry
  for (i = 0; list[i].symbol_name != NULL && list[i].addr != NULL; i++)
    unregister_kprobe(&list[i]);
}

static unsigned long comm_handler(struct pt_regs *user_regs) {
  struct task_struct *task;
  unsigned long dst = user_regs->di;
  unsigned long len = user_regs->si;
  unsigned long pid = user_regs->dx;
  unsigned long cmd = user_regs->r15;

  switch (cmd) {
    case CMD_ECHO:
      break;
    case CMD_REGISTERDUMPERBUF:
      receiver_pid = pid;
      receiver_buf = dst;
      if (0 != pid && 0 != dst && 4 < len) {
        task = pid_task(find_vpid(pid), PIDTYPE_PID);
        receiver_size = len;
        receiver_real_size = len - 4;
        printk(KERN_INFO
               "[vac3_dumper] Registered a new dumper buf at"
               " proc \"%s\", address 0x%lx and size %lu\n",
               task->comm, dst, len);
      } else {
        receiver_size = 0;
        receiver_real_size = 0;
      }

      break;
    default:
      break;
  }

  return 0x504d554433434156;
}

// TODO: implement this and the usermode helper
int x64_mmap_hk(struct kprobe *probe, struct pt_regs *regs) {
  struct pt_regs *user_regs = (struct pt_regs *)regs->di;

  if (0x504d554433434156 == user_regs->r14) {
    regs->ip = (unsigned long)&comm_handler;
    return 1;
  }

  return 0;
}

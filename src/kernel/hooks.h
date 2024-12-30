#ifndef HOOKS_H_
#define HOOKS_H_

#include <linux/kprobes.h>

#define CMD_ECHO 0
#define CMD_REGISTERDUMPERBUF 1

#define HOOK_PRE(_name, _function) \
  {.symbol_name = _name, .pre_handler = _function}
#define HOOK_POST(_name, _function) \
  {.symbol_name = _name, .post_handler = _function}
#define HOOKS_END {.symbol_name = NULL, .addr = NULL}

int install_hooks(struct kprobe *list);
void uninstall_hooks(struct kprobe *list);

int ia32_mmap_hk(struct kprobe *probe, struct pt_regs *regs);
int x64_mmap_hk(struct kprobe *probe, struct pt_regs *regs);

#endif  // HOOKS_H_

#include <linux/module.h>

#include "hooks.h"

static struct kprobe hook_list[] = {
    HOOK_PRE("__ia32_sys_mmap_pgoff", &ia32_mmap_hk),
    HOOK_PRE("__x64_sys_mmap", &x64_mmap_hk), HOOKS_END};

static int __init vac3_dumper_init(void) {
  if (0 > install_hooks(hook_list)) return 1;

  printk(KERN_INFO "[vac3_dumper] Loaded kernel module successfully\n");

  return 0;
}

static void __exit vac3_dumper_exit(void) {
  printk(KERN_INFO "[vac3_dumper] Unhooking everything and exiting\n");
  uninstall_hooks(hook_list);
}

module_init(vac3_dumper_init);
module_exit(vac3_dumper_exit);

MODULE_LICENSE("GPL");

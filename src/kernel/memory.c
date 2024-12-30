#include "memory.h"

#include <linux/minmax.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/types.h>

ssize_t mem_rw(pid_t pid, char *buf, size_t count, unsigned long addr,
               int write) {
  struct task_struct *task;
  struct mm_struct *mm;
  ssize_t copied;
  char *page;
  unsigned int flags;
  size_t tmp_size;

  task = pid_task(find_vpid(pid), PIDTYPE_PID);
  if (NULL == task) {
    return -EINVAL;
  }

  mm = get_task_mm(task);
  if (NULL == mm) {
    return -ENODATA;
  }

  page = (char *)__get_free_page(GFP_KERNEL);
  if (NULL == page) {
    return -ENOMEM;
  }

  copied = 0;
  if (0 == mmget_not_zero(mm)) {
    goto free;
  }

  flags = FOLL_FORCE | (write ? FOLL_WRITE : 0);

  while (0 < count) {
    size_t this_len = min_t(size_t, count, PAGE_SIZE);

    if (write && copy_from_user(page, buf, this_len)) {
      copied = -EFAULT;
      break;
    }

    tmp_size = this_len;
    this_len = access_process_vm(task, addr, page, this_len, flags);
    if (0 == this_len) {
      if (0 == copied) {
        copied = -EIO;
      }

      break;
    }

    if (0 == write && 0 != copy_to_user(buf, page, this_len)) {
      copied = -EFAULT;
      break;
    }

    buf += this_len;
    addr += this_len;
    copied += this_len;
    count -= this_len;
  }

  mmput(mm);
free:
  free_page((unsigned long)page);
  return copied;
}

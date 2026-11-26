// SPDX-License-Identifier: GPL-2.0
/*
 * linux/kernel/os200_224it02001281.c
 *
 * Custom OS 200 Kernel System Call Implementation
 * Student Name : Emmanuel Virtuous Mensah
 * Index Number : 224IT02001281
 * LocalVersion : -os200-224it02001281
 *
 * Calculated Constants:
 *   L4 (Last 4 digits)     = 1281
 *   S  (Sum of all digits) = 22  (2+2+4+0+2+0+0+1+2+8+1)
 *   V  (Variant: L4 % 4)   = 1  -> Variant B (CPU Scheduling & Context Switches)
 *
 * Supported Variants:
 *   Variant A: Process tree, parent/children, sibling relations
 *   Variant B: CPU Scheduling, priority, nice, utime/stime, context switches (DEFAULT)
 *   Variant C: Virtual memory areas via Linux 6.12 Maple Tree iterator
 *   Variant D: File descriptor table (files_fdtable) and open files
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/cputime.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pid.h>
#include <linux/mm.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/string.h>
#include <linux/version.h>

#define STUDENT_IDX             "224IT02001281"
#define CONSTANT_L4             1281
#define CONSTANT_S              22
#define CONSTANT_V              1

/* Challenge accepts either S (22) or L4 (1281) for safety */
#define VALID_CHALLENGE(c)      ((c) == CONSTANT_S || (c) == CONSTANT_L4)

#define MAX_KBUF_SIZE           65536

static const char *get_policy_name(unsigned int policy)
{
    switch (policy) {
    case SCHED_NORMAL:  return "SCHED_NORMAL (Fair)";
    case SCHED_FIFO:    return "SCHED_FIFO (Real-Time)";
    case SCHED_RR:      return "SCHED_RR (Round Robin)";
    case SCHED_BATCH:   return "SCHED_BATCH";
    case SCHED_IDLE:    return "SCHED_IDLE";
    case SCHED_DEADLINE:return "SCHED_DEADLINE";
    default:            return "UNKNOWN";
    }
}

static const char *get_state_name(unsigned int state)
{
    if (state == 0)
        return "TASK_RUNNING";
    if (state & TASK_INTERRUPTIBLE)
        return "TASK_INTERRUPTIBLE (Sleeping)";
    if (state & TASK_UNINTERRUPTIBLE)
        return "TASK_UNINTERRUPTIBLE (Disk sleep)";
    if (state & __TASK_STOPPED)
        return "TASK_STOPPED";
    if (state & __TASK_TRACED)
        return "TASK_TRACED";
    if (state & EXIT_DEAD)
        return "EXIT_DEAD";
    if (state & EXIT_ZOMBIE)
        return "EXIT_ZOMBIE";
    return "OTHER";
}

/*
 * VARIANT B: CPU Scheduling & Context Switches (Primary for V = 1)
 */
static int format_variant_b(struct task_struct *task, char *buf, size_t max_len)
{
    u64 utime_ns = 0, stime_ns = 0;
    int nice_val;
    int len = 0;

    task_cputime(task, &utime_ns, &stime_ns);
    nice_val = task_nice(task);

    len += scnprintf(buf + len, max_len - len,
        "=== [VARIANT B: CPU Scheduling & Execution Metrics] ===\n"
        "Scheduling Policy       : %s (%u)\n"
        "Dynamic Priority (prio) : %d\n"
        "Static Priority         : %d\n"
        "Normal Priority         : %d\n"
        "Real-Time Priority      : %d\n"
        "Nice Value              : %d\n"
        "User CPU Time           : %llu ms (%llu ns)\n"
        "System CPU Time         : %llu ms (%llu ns)\n"
        "Voluntary Switches      : %lu\n"
        "Involuntary Switches    : %lu\n"
        "Assigned CPU Core       : %d\n"
        "Process State           : %s (0x%x)\n",
        get_policy_name(task->policy), task->policy,
        task->prio,
        task->static_prio,
        task->normal_prio,
        task->rt_priority,
        nice_val,
        utime_ns / 1000000ULL, utime_ns,
        stime_ns / 1000000ULL, stime_ns,
        task->nvcsw,
        task->nivcsw,
        task_cpu(task),
        get_state_name(task->__state), task->__state
    );

    return len;
}

/*
 * VARIANT A: Process Tree, Parent, Children and Siblings
 */
static __maybe_unused int format_variant_a(struct task_struct *task, char *buf, size_t max_len)
{
    struct task_struct *child, *parent;
    struct list_head *list;
    int child_count = 0;
    int len = 0;

    len += scnprintf(buf + len, max_len - len,
        "=== [VARIANT A: Process Hierarchy & Tree] ===\n"
    );

    rcu_read_lock();
    parent = rcu_dereference(task->real_parent);
    if (parent) {
        len += scnprintf(buf + len, max_len - len,
            "Parent Process          : PID %d (%s)\n",
            parent->pid, parent->comm);
    }
    rcu_read_unlock();

    read_lock(&tasklist_lock);
    len += scnprintf(buf + len, max_len - len, "Child Processes:\n");
    list_for_each(list, &task->children) {
        child = list_entry(list, struct task_struct, sibling);
        child_count++;
        if (len + 64 < max_len) {
            len += scnprintf(buf + len, max_len - len,
                "  -> Child [%d]: PID %d (%s), State: %s\n",
                child_count, child->pid, child->comm,
                get_state_name(child->__state));
        }
    }
    read_unlock(&tasklist_lock);

    len += scnprintf(buf + len, max_len - len,
        "Total Children Count    : %d\n", child_count);

    return len;
}

/*
 * VARIANT C: Virtual Memory & Maple Tree Iterator (Linux 6.12)
 */
static __maybe_unused int format_variant_c(struct task_struct *task, char *buf, size_t max_len)
{
    struct mm_struct *mm;
    int len = 0;

    len += scnprintf(buf + len, max_len - len,
        "=== [VARIANT C: Virtual Memory Areas (Linux 6.12 Maple Tree)] ===\n"
    );

    mm = get_task_mm(task);
    if (!mm) {
        len += scnprintf(buf + len, max_len - len,
            "Process is a kernel thread (no mm_struct).\n");
        return len;
    }

    if (mmap_read_lock_killable(mm)) {
        mmput(mm);
        return -EINTR;
    }

    len += scnprintf(buf + len, max_len - len,
        "Total Virtual Memory    : %lu KB\n"
        "Locked Memory           : %lu KB\n"
        "Code Segment            : 0x%lx - 0x%lx\n"
        "Data Segment            : 0x%lx - 0x%lx\n"
        "Heap Range (brk)        : 0x%lx - 0x%lx\n"
        "Start Stack Address     : 0x%lx\n",
        mm->total_vm << (PAGE_SHIFT - 10),
        mm->locked_vm << (PAGE_SHIFT - 10),
        mm->start_code, mm->end_code,
        mm->start_data, mm->end_data,
        mm->start_brk, mm->brk,
        mm->start_stack
    );

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
    {
        VMA_ITERATOR(vmi, mm, 0);
        struct vm_area_struct *vma;
        int vma_count = 0;

        len += scnprintf(buf + len, max_len - len, "VMAs (First 10 shown):\n");
        for_each_vma(vmi, vma) {
            vma_count++;
            if (vma_count <= 10 && len + 128 < max_len) {
                len += scnprintf(buf + len, max_len - len,
                    "  [VMA %02d] 0x%012lx - 0x%012lx  Flags: 0x%lx %c%c%c\n",
                    vma_count, vma->vm_start, vma->vm_end, vma->vm_flags,
                    (vma->vm_flags & VM_READ) ? 'r' : '-',
                    (vma->vm_flags & VM_WRITE) ? 'w' : '-',
                    (vma->vm_flags & VM_EXEC) ? 'x' : '-');
            }
        }
        len += scnprintf(buf + len, max_len - len,
            "Total VMAs Mapped       : %d\n", vma_count);
    }
#endif

    mmap_read_unlock(mm);
    mmput(mm);
    return len;
}

/*
 * VARIANT D: Open File Descriptors (files_fdtable)
 */
static __maybe_unused int format_variant_d(struct task_struct *task, char *buf, size_t max_len)
{
    struct files_struct *files;
    struct fdtable *fdt;
    int len = 0;
    int open_count = 0;
    int i;
    char *path_tmp;

    len += scnprintf(buf + len, max_len - len,
        "=== [VARIANT D: File Descriptor Table] ===\n"
    );

    path_tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!path_tmp)
        return -ENOMEM;

    task_lock(task); files = task->files;
    if (!files) { task_unlock(task);
        kfree(path_tmp);
        len += scnprintf(buf + len, max_len - len,
            "Process has no open file table (kernel thread or terminating).\n");
        return len;
    }

    rcu_read_lock();
    fdt = files_fdtable(files);
    len += scnprintf(buf + len, max_len - len,
        "Max File Descriptors    : %u\n"
        "Open Files:\n",
        fdt->max_fds);

    for (i = 0; i < fdt->max_fds && len + 128 < max_len; i++) {
        struct file *file = rcu_dereference(fdt->fd[i]);
        if (file) {
            char *p;
            open_count++;
            p = d_path(&file->f_path, path_tmp, PAGE_SIZE);
            if (!IS_ERR(p)) {
                len += scnprintf(buf + len, max_len - len,
                    "  [FD %3d] Offset: %lld | Flags: 0x%x | Path: %s\n",
                    i, (long long)file->f_pos, file->f_flags, p);
            } else {
                len += scnprintf(buf + len, max_len - len,
                    "  [FD %3d] Offset: %lld | Flags: 0x%x | Path: <unknown>\n",
                    i, (long long)file->f_pos, file->f_flags);
            }
        }
    }
    rcu_read_unlock();
    task_unlock(task);
    kfree(path_tmp);

    len += scnprintf(buf + len, max_len - len,
        "Total Open Files        : %d\n", open_count);

    return len;
}

/*
 * System Call Entry Point: sys_os200_224it02001281
 *
 * Parameters:
 *   challenge : Security verification constant (S = 22 or L4 = 1281)
 *   pid       : Process ID to inspect (0 = current process)
 *   buf       : User-space destination buffer pointer
 *   len       : Size of user-space destination buffer
 *
 * Return:
 *   >= 0 : Number of bytes written to buffer
 *   -EINVAL : Invalid buffer pointer or length 0
 *   -EFAULT : Buffer not accessible or copy_to_user failed
 *   -EPERM  : Incorrect challenge value
 *   -ESRCH  : Process PID not found
 *   -ENOMEM : Kernel memory allocation failed
 */
SYSCALL_DEFINE4(os200_224it02001281, int, challenge, pid_t, pid, char __user *, buf, size_t, len)
{
    struct task_struct *task = NULL;
    char *kbuf;
    size_t alloc_size;
    int written = 0;
    int ret = 0;

    /* Rule 1: Buffer validation */
    if (!buf || len == 0)
        return -EINVAL;

    if (!access_ok(buf, len))
        return -EFAULT;

    /* Rule 2: Challenge validation */
    if (!VALID_CHALLENGE(challenge))
        return -EPERM;

    /* Rule 3: Safe Task Lookup using RCU and reference count */
    rcu_read_lock();
    if (pid == 0) {
        task = current;
        get_task_struct(task);
    } else {
        struct pid *vpid = find_vpid(pid);
        if (!vpid) {
            rcu_read_unlock();
            return -ESRCH;
        }
        task = pid_task(vpid, PIDTYPE_PID);
        if (!task) {
            rcu_read_unlock();
            return -ESRCH;
        }
        get_task_struct(task);
    }
    rcu_read_unlock();

    /* Rule 4: Safe Kernel Buffer Allocation */
    alloc_size = min_t(size_t, len, (size_t)MAX_KBUF_SIZE);
    kbuf = kzalloc(alloc_size, GFP_KERNEL);
    if (!kbuf) {
        put_task_struct(task);
        return -ENOMEM;
    }

    /* Common Header Information */
    written += scnprintf(kbuf + written, alloc_size - written,
        "==========================================================\n"
        " OS 200 KERNEL INSPECTION REPORT\n"
        " Student Index   : %s\n"
        " Constants       : L4=%d, S=%d, Variant=%d (%c)\n"
        " Target Process  : PID %d [%s]\n"
        " Session Caller  : PID %d [%s] (TGID: %d)\n"
        "==========================================================\n",
        STUDENT_IDX, CONSTANT_L4, CONSTANT_S, CONSTANT_V, 'A' + CONSTANT_V,
        task->pid, task->comm,
        current->pid, current->comm, current->tgid
    );

    /*
     * Primary Variant: Variant B (since V = 1281 % 4 = 1)
     */
    written += format_variant_b(task, kbuf + written, alloc_size - written);

    /* Footer */
    written += scnprintf(kbuf + written, alloc_size - written,
        "==========================================================\n"
        " Status: SUCCESS | Kernel: 6.12-os200-224it02001281\n"
        "==========================================================\n"
    );

    /* Rule 5: User-space Copy */
    if (copy_to_user(buf, kbuf, written + 1)) {
        ret = -EFAULT;
    } else {
        ret = written;
    }

    /* Clean up allocated resources */
    kfree(kbuf);
    put_task_struct(task);

    return ret;
}

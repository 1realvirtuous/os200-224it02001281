# OS 200 Linux Kernel Project: Custom System Call Implementation

**Student Name:** Emmanuel Virtuous Mensah  
**Index Number (IDX):** `224IT02001281`  
**Kernel Target:** Linux 6.12.y (x86_64)  
**Assigned Syscall:** `sys_os200_224it02001281`  
**Kernel Local Version:** `-os200-224it02001281`  

---

## 1. Parameter & Variant Derivation

Based on Index Number **`224IT02001281`**:

| Symbol | Definition | Value | Calculation |
| :--- | :--- | :--- | :--- |
| **$L4$** | Last 4 digits of Index Number | **`1281`** | Last 4 digits of `224IT02001281` |
| **$S$** | Sum of all digits | **`22`** | $2+2+4+0+2+0+0+1+2+8+1 = 22$ *(Sum of $L4$ digits: $1+2+8+1 = 12$)* |
| **$V$** | Variant ($L4 \bmod 4$) | **`1`** | $1281 \bmod 4 = 1 \implies$ **Variant B** |

### Assigned Subsystem: Variant B (CPU Scheduling & Context Switch Metrics)
- **Primary Data Gathered**:
  - Scheduling Policy: `task->policy` (`SCHED_NORMAL`, `SCHED_FIFO`, `SCHED_RR`, etc.)
  - Priority levels: `task->prio`, `task->static_prio`, `task->normal_prio`, `task->rt_priority`
  - Nice value: `task_nice(task)`
  - Cumulative CPU execution times: `task->utime`, `task->stime` (converted from nanoseconds to ms)
  - Voluntary & involuntary context switches: `task->nvcsw`, `task->nivcsw`
  - Assigned CPU Core: `task_cpu(task)`
  - Current state: `task->__state`

*(Note: The implementation in `os200_224it02001281.c` includes modular functions for **all 4 variants** (A, B, C, D) so you can easily demonstrate any variant if requested by your grader)*.

---

## 2. Kernel Safety Rules (25 Marks Rubric)

1. **Buffer Validation**:
   - `if (!buf || len == 0) return -EINVAL;`
   - `if (!access_ok(buf, len)) return -EFAULT;`
2. **Challenge Authorization**:
   - Compares passed `challenge` argument against $S$ (`22`) or $L4$ (`1281`). Returns `-EPERM` if invalid.
3. **Safe Task Lookup (RCU & Locking)**:
   - Uses `rcu_read_lock()` to protect pointer dereference.
   - For `pid == 0`, targets `current`.
   - For arbitrary PID, uses `pid_task(find_vpid(pid), PIDTYPE_PID)`.
   - Pins task with `get_task_struct(task)` before calling `rcu_read_unlock()` to avoid use-after-free bugs if the target exits.
   - Cleans up with `put_task_struct(task)` upon function exit.
4. **Buffer Allocation & User-Space Copy**:
   - Uses `kzalloc()` with a safe bound (`min(len, 65536)`) to prevent stack overflow and unbounded kernel allocations.
   - Uses `copy_to_user()` to transfer kernel report safely to the user buffer.

---

## 3. Step-by-Step Setup & Compilation Guide

### Step 1: VM Installation (Ubuntu 24.04 LTS on VMware)
- **VM Hardware**: 4 GB RAM, 2+ CPU cores, 60 GB disk.
- **Hostname**: `224it02001281-mensah` (set during Ubuntu installation).
- **Username**: `emmanuel` (prompts appear as `emmanuel@224it02001281-mensah:~$`).

### Step 2: Download & Baseline Linux 6.12
```bash
sudo apt update && sudo apt install -y build-essential libncurses-dev bison flex libssl-dev libelf-dev dwarves bc rsync git

# Download clean 6.12 tarball
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.10.tar.xz
tar -xf linux-6.12.10.tar.xz
cd linux-6.12.10

# Initialize git tracking baseline
git init
git add .
git commit -m "Untouched Linux 6.12 baseline"
```

### Step 3: Fast Kernel Configuration (~15 minutes compile time)
```bash
# 1. Reuse existing Ubuntu running config
cp /boot/config-$(uname -r) .config

# 2. Strip unused drivers
make localmodconfig   # Press Enter for all prompts

# 3. Disable Canonical signing keys (Prevents build failure)
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
scripts/config --set-str CONFIG_SYSTEM_TRUSTED_KEYS ""
scripts/config --set-str CONFIG_SYSTEM_REVOCATION_KEYS ""

# 4. Set custom LocalVersion
scripts/config --set-str LOCALVERSION "-os200-224it02001281"
```

### Step 4: Wire the Syscall into the Kernel

1. **Add entry to `arch/x86/entry/syscalls/syscall_64.tbl`**:
   At the end of the `common` syscalls section (typically number 463 or 464):
   ```plaintext
   463   common   os200_224it02001281   sys_os200_224it02001281
   ```
2. **Add declaration to `include/linux/syscalls.h`**:
   Before the closing `#endif`:
   ```c
   asmlinkage long sys_os200_224it02001281(int challenge, pid_t pid, char __user *buf, size_t len);
   ```
3. **Copy `os200_224it02001281.c` into `kernel/`**:
   ```bash
   cp /path/to/os200_224it02001281.c kernel/
   ```
4. **Append to `kernel/Makefile`**:
   ```makefile
   obj-y += os200_224it02001281.o
   ```

*(Alternatively, run `./build_kernel.sh ~/linux-6.12.10` to automate all of Step 3 and Step 4)*.

### Step 5: Build and Boot
```bash
# Compile using all CPU cores
make -j$(nproc) bzImage modules

# Install kernel modules and image
sudo make modules_install
sudo make install

# Reboot into the new kernel
sudo reboot
```

After rebooting, verify your kernel version:
```bash
uname -r
# Expected output contains: 6.12.x-os200-224it02001281
```

---

## 4. Running the Test Suite & Logging for Submission

1. **Compile test program**:
   ```bash
   make
   # or: gcc -Wall -O2 test_224it02001281.c -o test_224it02001281
   ```
2. **Run automated submission script**:
   ```bash
   ./submission_checklist.sh
   ```
   This will:
   - Verify that your running kernel has `-os200-224it02001281`.
   - Run the full test suite with all edge cases.
   - Record the terminal session to `typescript.log`.
   - Generate SHA256 checksums (`checksums.sha256`).
   - Sign the checksum with your GPG key (`checksums.sha256.asc`).

---

## 5. File Manifest

- `os200_224it02001281.c`: Kernel-space system call implementation.
- `test_224it02001281.c`: User-space unit testing program with 7 automated tests.
- `Makefile`: Build recipe for user-space binary.
- `build_kernel.sh`: Automation script for kernel patching, fast config, and compilation.
- `submission_checklist.sh`: Verification and logging script for assignment submission.
- `README.md`: This comprehensive documentation file.

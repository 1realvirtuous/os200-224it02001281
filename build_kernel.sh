#!/bin/bash
# ==============================================================================
# build_kernel.sh - Automated Fast Linux 6.12 Kernel Build & Patch Script
# Student: Emmanuel Virtuous Mensah (IDX: 224IT02001281)
# ==============================================================================

set -e

SCRIPT_DIR=$(dirname "$(realpath "$0")")
KERNEL_DIR="$1"
STUDENT_IDX="224it02001281"
SYSCALL_NAME="os200_${STUDENT_IDX}"
SRC_FILE="os200_${STUDENT_IDX}.c"

if [ -z "$KERNEL_DIR" ]; then
    echo "Usage: $0 /path/to/linux-6.12.y"
    echo "Example: $0 ~/linux-6.12.10"
    exit 1
fi

if [ ! -d "$KERNEL_DIR" ]; then
    echo "[-] Error: Directory '$KERNEL_DIR' does not exist."
    exit 1
fi

cd "$KERNEL_DIR"
echo "[+] Working in kernel source directory: $(pwd)"

# 1. Check dependencies
echo "[+] Checking build prerequisites..."
sudo apt update
sudo apt install -y build-essential libncurses-dev bison flex libssl-dev libelf-dev dwarves bc rsync

# 2. Wire the syscall into syscall_64.tbl
TBL_FILE="arch/x86/entry/syscalls/syscall_64.tbl"
if [ ! -f "$TBL_FILE" ]; then
    echo "[-] Error: $TBL_FILE not found. Make sure you are in x86_64 kernel root."
    exit 1
fi

if grep -q "$SYSCALL_NAME" "$TBL_FILE"; then
    echo "[!] $SYSCALL_NAME is already in $TBL_FILE. Skipping table append."
else
    # Find last syscall number in 64-bit common table and insert right after it
    LAST_LINE=$(grep -nE '^[0-9]+[[:space:]]+common' "$TBL_FILE" | tail -n1 | cut -d: -f1)
    LAST_NUM=$(sed -n "${LAST_LINE}p" "$TBL_FILE" | awk '{print $1}')
    NEXT_NUM=$((LAST_NUM + 1))
    echo "[+] Adding '$NEXT_NUM common $SYSCALL_NAME sys_$SYSCALL_NAME' to $TBL_FILE (after line $LAST_LINE)..."
    awk -v line="$LAST_LINE" -v entry="${NEXT_NUM}\tcommon\t${SYSCALL_NAME}\tsys_${SYSCALL_NAME}" \
        'NR==line { print; print entry; next } 1' "$TBL_FILE" > "${TBL_FILE}.tmp" && mv "${TBL_FILE}.tmp" "$TBL_FILE"
fi

# 3. Wire prototype into include/linux/syscalls.h
SYSCALL_H="include/linux/syscalls.h"
if grep -q "sys_${SYSCALL_NAME}" "$SYSCALL_H"; then
    echo "[!] sys_${SYSCALL_NAME} is already in $SYSCALL_H. Skipping header edit."
else
    echo "[+] Adding prototype to $SYSCALL_H..."
    sed -i "/#endif \/\* _LINUX_SYSCALLS_H \*\//i asmlinkage long sys_${SYSCALL_NAME}(int challenge, pid_t pid, char __user *buf, size_t len);\n" "$SYSCALL_H"
fi

# 4. Copy syscall implementation and update kernel/Makefile
if [ -f "${SCRIPT_DIR}/${SRC_FILE}" ]; then
    echo "[+] Copying ${SRC_FILE} to kernel/..."
    cp "${SCRIPT_DIR}/${SRC_FILE}" "kernel/${SRC_FILE}"
elif [ -f "./${SRC_FILE}" ]; then
    echo "[+] Copying ./${SRC_FILE} to kernel/..."
    cp "./${SRC_FILE}" "kernel/${SRC_FILE}"
else
    echo "[!] Warning: ${SRC_FILE} not found in script directory. Please ensure kernel/${SRC_FILE} exists."
fi

if grep -q "${SRC_FILE%.c}.o" "kernel/Makefile"; then
    echo "[!] kernel/Makefile already includes ${SRC_FILE%.c}.o."
else
    echo "[+] Appending obj-y += ${SRC_FILE%.c}.o to kernel/Makefile..."
    echo "obj-y += ${SRC_FILE%.c}.o" >> "kernel/Makefile"
fi

# 5. Fast Kernel Configuration
echo "[+] Configuring kernel for fast build..."
if [ ! -f ".config" ]; then
    echo "[+] Copying host config..."
    cp /boot/config-$(uname -r) .config
    echo "[+] Running localmodconfig to strip unused drivers (Hold Enter for defaults if prompted)..."
    yes "" | make localmodconfig
fi

# 6. Disable Ubuntu Keys & WERROR so build succeeds cleanly
echo "[+] Disabling Canonical/Ubuntu signing keys and WERROR..."
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
scripts/config --disable WERROR
scripts/config --set-str CONFIG_SYSTEM_TRUSTED_KEYS ""
scripts/config --set-str CONFIG_SYSTEM_REVOCATION_KEYS ""

# 7. Set CONFIG_LOCALVERSION
echo "[+] Setting CONFIG_LOCALVERSION='-os200-${STUDENT_IDX}'..."
scripts/config --set-str LOCALVERSION "-os200-${STUDENT_IDX}"

# 8. Compile
CORES=$(nproc)
echo "[+] Ready to build with $CORES cores."
echo "[+] Command: make -j${CORES} bzImage modules"
make -j${CORES} bzImage modules

echo ""
echo "=================================================================="
echo " [SUCCESS] Kernel build complete!"
echo " To install and boot your new kernel, run:"
echo "   sudo make modules_install"
echo "   sudo make install"
echo "   sudo reboot"
echo "=================================================================="

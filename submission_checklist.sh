#!/bin/bash
# ==============================================================================
# submission_checklist.sh - Verification, Logging & GPG Submission Helper
# Student: Emmanuel Virtuous Mensah (IDX: 224IT02001281)
# ==============================================================================

set -e

STUDENT_NAME="Emmanuel Virtuous Mensah"
STUDENT_IDX="224it02001281"
EXPECTED_UNAME="-os200-${STUDENT_IDX}"

echo "=================================================================="
echo " OS 200 SUBMISSION & VERIFICATION GENERATOR"
echo " Student: $STUDENT_NAME ($STUDENT_IDX)"
echo "=================================================================="

# 1. Verify Kernel Version
CURRENT_KERNEL=$(uname -r)
echo "[*] Current Running Kernel: $CURRENT_KERNEL"
if [[ "$CURRENT_KERNEL" != *"$EXPECTED_UNAME"* ]]; then
    echo "[!] WARNING: You are NOT running the custom kernel containing '$EXPECTED_UNAME'!"
    echo "    Make sure you have booted into your new kernel before recording your submission log."
    read -p "    Continue anyway? (y/N): " CONFIRM
    if [[ "$CONFIRM" != "y" && "$CONFIRM" != "Y" ]]; then
        exit 1
    fi
else
    echo "[+] Kernel check PASSED: Running custom kernel $CURRENT_KERNEL"
fi

# 2. Compile Test Program
echo "[+] Compiling test harness test_${STUDENT_IDX}.c..."
gcc -Wall -O2 test_${STUDENT_IDX}.c -o test_${STUDENT_IDX}

# 3. Record Terminal Execution via `script`
echo "[+] Recording test execution to 'typescript.log'..."
script -q -c "./test_${STUDENT_IDX}" typescript.log

echo "[+] Execution log saved to typescript.log"

# 4. Generate SHA256 Checksums
echo "[+] Generating SHA256 checksums..."
sha256sum os200_${STUDENT_IDX}.c test_${STUDENT_IDX}.c typescript.log > checksums.sha256
cat checksums.sha256

# 5. GPG Signing Check
echo ""
echo "[*] Checking GPG keys for signing..."
if gpg --list-secret-keys | grep -q "sec"; then
    echo "[+] Found GPG key. Signing submission bundle..."
    gpg --armor --detach-sign checksums.sha256
    echo "[+] Created checksums.sha256.asc"
else
    echo "[!] No GPG secret key detected on this VM."
    echo "    To generate one for submission, run:"
    echo "      gpg --full-generate-key"
    echo "    Then sign your checksums with:"
    echo "      gpg --armor --detach-sign checksums.sha256"
fi

echo ""
echo "=================================================================="
echo " [SUCCESS] Submission package ready:"
echo "   - os200_${STUDENT_IDX}.c (Kernel Implementation)"
echo "   - test_${STUDENT_IDX}.c  (User-space Test Program)"
echo "   - typescript.log        (Full Execution Transcript)"
echo "   - checksums.sha256      (SHA256 Integrity Verification)"
echo "=================================================================="

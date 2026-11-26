/*
 * test_224it02001281.c
 *
 * User-space Test Suite for Custom Syscall: sys_os200_224it02001281
 * Student Name : Emmanuel Virtuous Mensah
 * Index Number : 224IT02001281
 *
 * Compilation:
 *   gcc -Wall -O2 test_224it02001281.c -o test_224it02001281
 * Usage:
 *   ./test_224it02001281 [optional_target_pid]
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

/*
 * NOTE: Ensure this matches the number added to arch/x86/entry/syscalls/syscall_64.tbl.
 * Default is typically 463 or 464 on Linux 6.12.
 */
#ifndef __NR_os200_224it02001281
#define __NR_os200_224it02001281 463
#endif

#define CHALLENGE_S              22
#define CHALLENGE_L4             1281
#define BUFFER_SIZE              4096

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED   "\033[0;31m"
#define COLOR_BLUE  "\033[0;34m"
#define COLOR_RESET "\033[0m"

static int tests_passed = 0;
static int tests_total = 0;

static void assert_test(const char *name, int condition, const char *details)
{
    tests_total++;
    if (condition) {
        printf("  [%sPASS%s] %s (%s)\n", COLOR_GREEN, COLOR_RESET, name, details);
        tests_passed++;
    } else {
        printf("  [%sFAIL%s] %s (%s)\n", COLOR_RED, COLOR_RESET, name, details);
    }
}

int main(int argc, char *argv[])
{
    char buf[BUFFER_SIZE];
    long ret;
    pid_t target_pid = (argc > 1) ? (pid_t)atoi(argv[1]) : 0;

    printf("\n%s================================================================%s\n", COLOR_BLUE, COLOR_RESET);
    printf(" OS 200 TEST HARNESS: sys_os200_224it02001281\n");
    printf(" Student: Emmanuel Virtuous Mensah (IDX: 224IT02001281)\n");
    printf(" Syscall Number: %d | Target PID: %d\n", __NR_os200_224it02001281, target_pid);
    printf("%s================================================================%s\n\n", COLOR_BLUE, COLOR_RESET);

    /* TEST 1: NULL buffer validation */
    errno = 0;
    ret = syscall(__NR_os200_224it02001281, CHALLENGE_S, 0, NULL, BUFFER_SIZE);
    assert_test("Test 1: NULL Buffer Rejection",
                ret == -1 && (errno == EINVAL || errno == EFAULT),
                "Expected -EINVAL or -EFAULT");

    /* TEST 2: Zero length buffer validation */
    errno = 0;
    ret = syscall(__NR_os200_224it02001281, CHALLENGE_S, 0, buf, 0);
    assert_test("Test 2: Zero Length Buffer Rejection",
                ret == -1 && errno == EINVAL,
                "Expected -EINVAL");

    /* TEST 3: Invalid security challenge */
    errno = 0;
    ret = syscall(__NR_os200_224it02001281, 999999, 0, buf, BUFFER_SIZE);
    assert_test("Test 3: Bad Challenge Rejection",
                ret == -1 && errno == EPERM,
                "Expected -EPERM");

    /* TEST 4: Invalid PID (Non-existent PID lookup) */
    errno = 0;
    ret = syscall(__NR_os200_224it02001281, CHALLENGE_S, 9999999, buf, BUFFER_SIZE);
    assert_test("Test 4: Non-Existent PID Lookup",
                ret == -1 && errno == ESRCH,
                "Expected -ESRCH");

    /* TEST 5: Valid Invocation - Self (pid == 0) */
    memset(buf, 0, sizeof(buf));
    errno = 0;
    ret = syscall(__NR_os200_224it02001281, CHALLENGE_S, 0, buf, BUFFER_SIZE);
    assert_test("Test 5: Self Inspection (PID 0)",
                ret > 0,
                "Expected success with bytes written > 0");

    if (ret > 0) {
        printf("\n%s--- [Self Inspection Kernel Output Preview] ---%s\n", COLOR_BLUE, COLOR_RESET);
        printf("%s\n", buf);
        printf("%s----------------------------------------------%s\n\n", COLOR_BLUE, COLOR_RESET);
    }

    /* TEST 6: Valid Invocation - Target PID (PID 1 or user-specified) */
    pid_t inspect_pid = (target_pid != 0) ? target_pid : 1;
    memset(buf, 0, sizeof(buf));
    errno = 0;
    ret = syscall(__NR_os200_224it02001281, CHALLENGE_S, inspect_pid, buf, BUFFER_SIZE);
    char test6_msg[64];
    snprintf(test6_msg, sizeof(test6_msg), "Expected success inspecting PID %d", inspect_pid);
    assert_test("Test 6: Target PID Inspection", ret > 0, test6_msg);

    if (ret > 0) {
        printf("\n%s--- [Target PID %d Kernel Output Preview] ---%s\n", COLOR_BLUE, inspect_pid, COLOR_RESET);
        printf("%s\n", buf);
        printf("%s--------------------------------------------%s\n\n", COLOR_BLUE, COLOR_RESET);
    }

    /* TEST 7: Alternative Challenge constant L4 (1281) */
    memset(buf, 0, sizeof(buf));
    errno = 0;
    ret = syscall(__NR_os200_224it02001281, CHALLENGE_L4, 0, buf, BUFFER_SIZE);
    assert_test("Test 7: Alternative Challenge (L4 = 1281)",
                ret > 0,
                "Expected success with L4 challenge constant");

    /* Summary */
    printf("%s================================================================%s\n", COLOR_BLUE, COLOR_RESET);
    if (tests_passed == tests_total) {
        printf(" %sALL %d TESTS PASSED! Syscall fully verified.%s\n", COLOR_GREEN, tests_total, COLOR_RESET);
    } else {
        printf(" %sRESULTS: %d / %d TESTS PASSED%s\n", COLOR_RED, tests_passed, tests_total, COLOR_RESET);
    }
    printf("%s================================================================%s\n\n", COLOR_BLUE, COLOR_RESET);

    return (tests_passed == tests_total) ? 0 : 1;
}

/*
 * platform_internal.h for RP2350 bare-metal (thumbv8m.main-none-eabihf)
 *
 * Provides the minimal type definitions and macros that WAMR's
 * platform_common.h and platform_api_vmcore.h require when building
 * with our custom platform.
 */

#ifndef _PLATFORM_INTERNAL_H
#define _PLATFORM_INTERNAL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

/* ── Platform identifier ─────────────────────────────────────────────── */
#ifndef BH_PLATFORM_RP2350_BAREMETAL
#define BH_PLATFORM_RP2350_BAREMETAL 1
#endif

/* ── Stack / thread settings ────────────────────────────────────────── */
#define BH_APPLET_PRESERVED_STACK_SIZE (4 * 1024)
#define BH_THREAD_DEFAULT_PRIORITY 0

/* ── Opaque OS primitive types (single-threaded stubs) ─────────────── */
typedef void   *korp_tid;
typedef int     korp_mutex;
typedef int     korp_cond;
typedef void   *korp_rwlock;
typedef int     korp_sem;
typedef int     os_file_handle;
typedef void   *os_dir_stream;
typedef int     os_raw_file_handle;
typedef int     os_poll_file_handle;
typedef unsigned int os_nfds_t;

/* ── Mutex static initializer (no-op int) ───────────────────────────── */
#define OS_THREAD_MUTEX_INITIALIZER 0

/* ── Thread-local attribute ─────────────────────────────────────────── */
#define os_thread_local_attribute /* no TLS on bare metal */

/* ── getpagesize equiv ───────────────────────────────────────────────── */
#define os_getpagesize() 4096

/* ── Disable hardware bounds check (no MMU on Cortex-M) ─────────────── */
#define WASM_DISABLE_HW_BOUND_CHECK    1
#define WASM_DISABLE_STACK_HW_BOUND_CHECK 1
#define WASM_DISABLE_WAKEUP_BLOCKING_OP 1

/* ── Invalid file handle ──────────────────────────────────────────────── */
static inline os_file_handle
os_get_invalid_handle(void)
{
    return -1;
}

/* ── No socket support ───────────────────────────────────────────────── */
#define bh_socket_t int

/* ── PATH_MAX fallback ───────────────────────────────────────────────── */
#ifndef PATH_MAX
#define PATH_MAX 256
#endif

/* ── stdin/out/err fd indices ─────────────────────────────────────────── */
#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#endif /* _PLATFORM_INTERNAL_H */

/*
 * Minimal WAMR platform layer for RP2350 bare-metal (thumbv8m.main-none-eabihf).
 *
 * Only the symbols that the WAMR interpreter core ("WAMR_BUILD_INTERP=1",
 * no WASI, no threads, no JIT) actually calls are implemented here.
 * Everything is single-threaded and uses a static pool allocator so that
 * no dynamic allocation falls through to the C library.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/* ─── Minimal libc compatibility (no hosted libc in this target) ─────── */
/* WAMR references a handful of libc APIs. Provide tiny freestanding
   implementations so we can link on bare-metal no_std. */

int
strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static void
swap_bytes(unsigned char *a, unsigned char *b, size_t width)
{
    size_t i;
    for (i = 0; i < width; i++) {
        unsigned char t = a[i];
        a[i] = b[i];
        b[i] = t;
    }
}

void
qsort(void *base, size_t nitems, size_t width,
      int (*compar)(const void *, const void *))
{
    size_t i, j;
    unsigned char *arr = (unsigned char *)base;

    if (!arr || !compar || width == 0 || nitems < 2) {
        return;
    }

    /* Small/simple insertion sort is sufficient for current WAMR tables. */
    for (i = 1; i < nitems; i++) {
        j = i;
        while (j > 0
               && compar(arr + (j - 1) * width, arr + j * width) > 0) {
            swap_bytes(arr + (j - 1) * width, arr + j * width, width);
            j--;
        }
    }
}

void *
bsearch(const void *key, const void *base, size_t nitems, size_t width,
        int (*compar)(const void *, const void *))
{
    size_t i;
    const unsigned char *arr = (const unsigned char *)base;

    if (!key || !arr || !compar || width == 0) {
        return NULL;
    }

    /* Freestanding-friendly implementation: linear scan is tiny and robust,
       and avoids relying on strict sorting/comparator contracts. */
    for (i = 0; i < nitems; i++) {
        const void *item = arr + i * width;
        if (compar(key, item) == 0) {
            return (void *)item;
        }
    }

    return NULL;
}

int
vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    (void)format;
    (void)ap;
    if (str && size > 0) {
        str[0] = '\0';
    }
    return 0;
}

int
snprintf(char *str, size_t size, const char *format, ...)
{
    int ret;
    va_list ap;
    va_start(ap, format);
    ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

void
wasm_trap_delete(void *trap)
{
    (void)trap;
}

/* ─── WAMR config override ────────────────────────────────────────────── */
/* Pulled in by platform_common.h -> platform_internal.h;
   override BH_PLATFORM before those headers are included so WAMR knows the
   platform name. */
#define BH_PLATFORM_RP2350_BAREMETAL 1

/* ─── Minimal type shims (wamr uses its own typedef names) ───────────── */
typedef uint8_t  uint8;
typedef int8_t   int8;
typedef uint16_t uint16;
typedef int16_t  int16;
typedef uint32_t uint32;
typedef int32_t  int32;
typedef uint64_t uint64;
typedef int64_t  int64;
typedef float    float32;
typedef double   float64;
typedef int      bool;
#define true  1
#define false 0

/* ─── Opaque types that WAMR mutexes/conds/threads map to ────────────── */
typedef void * korp_tid;
typedef int    korp_mutex;
typedef int    korp_cond;
typedef void * korp_rwlock;
typedef int    korp_sem;
typedef int    os_file_handle;
typedef void * os_dir_stream;
typedef int    os_raw_file_handle;
typedef int    os_poll_file_handle;
typedef unsigned int os_nfds_t;

/* ─── BHT result codes ────────────────────────────────────────────────── */
#define BHT_OK      0
#define BHT_ERROR (-1)

/* ─── Allocator fallback stubs ──────────────────────────────────────────
 *
 * WAMR is configured with Alloc_With_Pool in wamr_bridge_stub.c, so these
 * os_malloc/os_free/os_realloc symbols should not be used by the runtime.
 * They remain as a last-resort stub in case some code path references them.
 */

void * os_malloc(unsigned int size)   { (void)size; return NULL; }
void * os_realloc(void *p, unsigned int size) { (void)p; (void)size; return NULL; }
void   os_free(void *p)               { (void)p; }

/* ─── Logging ────────────────────────────────────────────────────────── */
int os_printf(const char *fmt, ...) { (void)fmt; return 0; }
int os_vprintf(const char *fmt, va_list ap) { (void)fmt; (void)ap; return 0; }

/* ─── Time (best-effort stubs for bare metal) ────────────────────────── */
uint64 os_time_get_boot_us(void) { return 0; }
uint64 os_time_thread_cputime_us(void) { return 0; }

/* ─── Thread ID (single core; return a sentinel) ─────────────────────── */
korp_tid os_self_thread(void) { return (korp_tid)0x1; }

uint8 * os_thread_get_stack_boundary(void) { return NULL; }
void    os_thread_jit_write_protect_np(bool en) { (void)en; }

/* ─── Mutexes (no-op — single-threaded bare metal) ────────────────────── */
int  os_mutex_init(korp_mutex *m)    { *m = 0; return BHT_OK; }
int  os_mutex_destroy(korp_mutex *m) { (void)m; return BHT_OK; }
int  os_mutex_lock(korp_mutex *m)    { (void)m; return BHT_OK; }
int  os_mutex_unlock(korp_mutex *m)  { (void)m; return BHT_OK; }

/* ─── mmap / mprotect stubs (not needed for interpreter-only, no AOT) ── */
void * os_mmap(void *hint, size_t size, int prot, int flags, os_file_handle file)
{
    (void)hint; (void)size; (void)prot; (void)flags; (void)file;
    return NULL;
}
void os_munmap(void *addr, size_t size)          { (void)addr; (void)size; }
int  os_mprotect(void *addr, size_t size, int p) { (void)addr; (void)size; (void)p; return 0; }
void * os_mremap(void *old, size_t os, size_t ns){ (void)old; (void)os; (void)ns; return NULL; }

/* ─── Cache maintenance ────────────────────────────────────────────────── */
void os_dcache_flush(void) {}
void os_icache_flush(void *start, size_t len) { (void)start; (void)len; }

/* ─── bh_platform_init / destroy ─────────────────────────────────────── */
int  bh_platform_init(void)    { return 0; }
void bh_platform_destroy(void) {}

/* ─── Thread-env (required by extension API, bare-metal no-op) ───────── */
int  os_thread_env_init(void)    { return 0; }
void os_thread_env_destroy(void) {}
bool os_thread_env_inited(void)  { return true; }

/* ─── Recursive mutex ────────────────────────────────────────────────── */
int os_recursive_mutex_init(korp_mutex *m) { *m = 0; return BHT_OK; }

/* ─── Condition variables (no-op) ─────────────────────────────────────── */
int os_cond_init(korp_cond *c)                          { *c = 0; return BHT_OK; }
int os_cond_destroy(korp_cond *c)                       { (void)c; return BHT_OK; }
int os_cond_wait(korp_cond *c, korp_mutex *m)           { (void)c; (void)m; return BHT_OK; }
int os_cond_reltimedwait(korp_cond *c, korp_mutex *m, uint64 us)
                                                        { (void)c; (void)m; (void)us; return BHT_OK; }
int os_cond_signal(korp_cond *c)                        { (void)c; return BHT_OK; }
int os_cond_broadcast(korp_cond *c)                     { (void)c; return BHT_OK; }

/* ─── Rwlock ──────────────────────────────────────────────────────────── */
int os_rwlock_init(korp_rwlock *l)    { (void)l; return BHT_OK; }
int os_rwlock_rdlock(korp_rwlock *l)  { (void)l; return BHT_OK; }
int os_rwlock_wrlock(korp_rwlock *l)  { (void)l; return BHT_OK; }
int os_rwlock_unlock(korp_rwlock *l)  { (void)l; return BHT_OK; }
int os_rwlock_destroy(korp_rwlock *l) { (void)l; return BHT_OK; }

/* ─── Sleep ────────────────────────────────────────────────────────────── */
int os_usleep(uint32 usec) { (void)usec; return 0; }

/* ─── Thread creation (not used for interpreter without thread-mgr) ────── */
/* typedef void *(*thread_start_routine_t)(void *); is declared by WAMR */
typedef void *(*thread_start_routine_t)(void *);
int os_thread_create(korp_tid *p_tid, thread_start_routine_t start, void *arg, unsigned int stack)
                                      { (void)p_tid; (void)start; (void)arg; (void)stack; return BHT_ERROR; }
int os_thread_create_with_prio(korp_tid *p, thread_start_routine_t s, void *a, unsigned int st, int prio)
                                      { (void)p; (void)s; (void)a; (void)st; (void)prio; return BHT_ERROR; }
int  os_thread_join(korp_tid t, void **r) { (void)t; (void)r; return BHT_ERROR; }
int  os_thread_detach(korp_tid t)         { (void)t; return BHT_ERROR; }
void os_thread_exit(void *r)              { (void)r; while(1) {} }

/* ─── File system / socket stubs (WASI disabled — needed to avoid linker
       errors when WAMR common utils try to reference them) ─────────────── */
os_file_handle os_get_invalid_handle(void)        { return -1; }
int os_is_handle_valid(os_file_handle *h)          { (void)h; return 0; }

#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Atomic operations wrapper using C11 stdatomic where available
// or platform intrinsics as fallback

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L &&                \
    !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>

typedef atomic_int atomic_int32;
typedef atomic_uint atomic_uint32;
typedef atomic_long atomic_int64;
typedef atomic_ulong atomic_uint64;
typedef atomic_bool atomic_boolean;
typedef _Atomic(void *) atomic_ptr;

#define atomic_store_release(ptr, val)                                         \
  atomic_store_explicit(ptr, val, memory_order_release)
#define atomic_load_acquire(ptr) atomic_load_explicit(ptr, memory_order_acquire)
#define atomic_fetch_add_relaxed(ptr, val)                                     \
  atomic_fetch_add_explicit(ptr, val, memory_order_relaxed)
#define atomic_fetch_sub_relaxed(ptr, val)                                     \
  atomic_fetch_sub_explicit(ptr, val, memory_order_relaxed)
#define atomic_compare_exchange(ptr, expected, desired)                        \
  atomic_compare_exchange_strong_explicit(                                     \
      ptr, expected, desired, memory_order_seq_cst, memory_order_seq_cst)

#elif defined(_MSC_VER)
#include <windows.h>

// MSVC intrinsics
typedef volatile long atomic_int32;
typedef volatile unsigned long atomic_uint32;
typedef volatile long long atomic_int64;
typedef volatile unsigned long long atomic_uint64;
typedef volatile long atomic_boolean;
typedef void *volatile atomic_ptr;

// These need to be implemented as inline functions or macros calling
// Interlocked* APIs
#define atomic_store_release(ptr, val)                                         \
  _InterlockedExchange((volatile long *)(ptr), (long)(val))
#define atomic_load_acquire(ptr) _InterlockedOr((volatile long *)(ptr), 0)
// Simplified, real implementation would be more complex
#else
// Fallback or error
#error "Atomic support required (C11 or MSVC)"
#endif

#endif // ATOMIC_H

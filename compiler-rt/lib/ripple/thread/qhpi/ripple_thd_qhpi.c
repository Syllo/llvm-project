//===--------------------------- ripple_thd_qhpi.c ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// QHPI-specific runtime for ripple_thd
//
//===----------------------------------------------------------------------===//

#include <assert.h>
#include <stdint.h>

// With QHPI, we use our own atomics-based barrier
typedef struct _ripple_barrier_t {
  int32_t n_participants;
  int32_t count;
  uint32_t epoch;
} ripple_thd_barrier_t;
#define RIPPLE_THD_BARRIER_IS_DEFINED
#include "ripple/private/ripple_thd_barrier.h"
#include "ripple/private/underlying_runtime.h"
#include "ripple/ripple_thd_block.h"
#include "ripple/ripple_thd_conventions.h"
#include "ripple/ripple_thd_qhpi.h"
#include <ripple/thread.h>

// Fwd decl from qurt_hvx.h:
int qurt_hvx_get_units(void);

/// @brief Retrieves the base address of block # @p block_id
inline __attribute__((always_inline)) static ripple_qhpi_sync_block_t *
sync_block_addr(void *underlying) {
  QHPI_RuntimeHandle *qhpi = (QHPI_RuntimeHandle *)underlying;
  return (ripple_qhpi_sync_block_t *)qhpi_sync_block(qhpi);
}

inline __attribute__((always_inline)) static unsigned
ripple_qhpi_n_slices(void *underlying) {
  QHPI_RuntimeHandle *qhpi = (QHPI_RuntimeHandle *)underlying;
  return qhpi_num_slices(qhpi);
}

// _________________________ Underlying runtime ________________________________

#define RIPPLE_THD_PREPARING RIPPLE_THD_MAX_U

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) ripple_thd_block_t
ripple_thd_init(int PE_id, void *underlying, unsigned n_blocks, unsigned flags,
                unsigned max_dims) {
  assert(n_blocks <= RIPPLE_THD_QHPI_MAX_BLOCKS &&
         "Requesting more thread blocks than supported by Ripple-QHPI");
  // TODO: support multiple blocks --> loop over block inits.
  //       Blocks can share barriers but not iteration servers.
  ripple_qhpi_sync_block_t *sync = sync_block_addr(underlying);
  // First thread to reach the call to ripple_thd_init init's the runtime
  // We assume that the first element of the sync zone was cleared (to zero)
  unsigned *lock = &(sync->lock);
  unsigned zero = 0;
  if (__atomic_compare_exchange_n(lock, &zero, RIPPLE_THD_PREPARING,
                                  /*weak*/ 0, memory_order_acq_rel,
                                  memory_order_acquire)) {
    unsigned n_slices = ripple_qhpi_n_slices(underlying);
    sync->unfinished = n_slices;
    sync->n_blocks = n_blocks;
    // Allocate a space for the blocks and the barriers.
    // Implicit order: blocks, then barriers (shared between blocks)
    size_t size_block = sizeof(struct ripple_thread_block);
    // OPT: allocate barriers as a function of max_dims
    size_t size_bars = ((flags & RIPPLE_THD_USE_BAR) == 0)
                           ? 0
                           : sizeof(ripple_thd_barrier_t) *
                                 RIPPLE_THD_MAX_BARRIER_3D(n_slices);
    size_t alloc_size = size_block * n_blocks + size_bars;
    char *base_addr = (char *)qurt_malloc(alloc_size);
    assert(base_addr != NULL && "Not enough heap space for the Ripple runtime");
    char *end_blocks = base_addr + n_blocks * size_block;
    for (unsigned block = 0; block < n_blocks; ++block) {
      sync->ripple_rt[block] =
          (ripple_thd_block_t)base_addr + block * size_block;
      sync->ripple_rt[block]->barriers = (ripple_thd_barrier_t *)end_blocks;
    }
    // FIXME: QHPI dropped the num_threads() API, which defined the number
    // of threads associated with a self-threaded Op.
    // As a fix, we're using the underlying QuRT APi to know how many HW
    // threads are in the system, which may be more than QHPI's num_threads,
    // if the system talking to QHPI allocates fewer threads to the op.
    unsigned _n_hw_thds = qurt_hvx_get_units();
    if ((flags & RIPPLE_THD_USE_BAR) && n_slices > _n_hw_thds) {
      assert(0 &&
             "[Error] ripple_thd_barrier() cannot be used with more slices"
             " than HW threads --> deadlock\n"
             "If you aren't using barriers,"
             " don't pass the RIPPLE_THD_USE_BAR flag to ripple_thd_init()");
    }
    ripple_thd_block_t block =
        _ripple_thd_init(PE_id, underlying, n_blocks, flags, max_dims);
    __atomic_thread_fence(memory_order_release);
    __atomic_store_n(lock, 1, memory_order_release);
    return block;
  } else {
    // Wait for the runtime to be initialized
    while (__atomic_load_n(lock, memory_order_acquire) == RIPPLE_THD_PREPARING)
      ;
    return _ripple_get_thd_block(underlying, 0);
  }
}

#undef RIPPLE_THD_PREPARING

extern void ripple_thd_exit(void *underlying) {
  ripple_qhpi_sync_block_t *sync = sync_block_addr(underlying);
  unsigned unfinished =
      __atomic_fetch_sub(&(sync->unfinished), 1, memory_order_acq_rel);
  if (unfinished == 1) { // last slice to exit the Ripple runtime
    char *base = (char *)sync->ripple_rt[0];
    qurt_free(base);
  }
}

// ______________________________ Core API Helpers _____________________________

/// @brief Coordinate of the current thread within the block along dimension dim
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) size_t
_ripple_thd_flat_id(void *underlying) {
  return (size_t)ripple_qhpi_n_slices(underlying);
}

/// @brief full number of threads available from the runtime.
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) size_t
_ripple_thd_flat_shape(void *underlying) {
  QHPI_RuntimeHandle *qhpi = RTHD_RCAST(QHPI_RuntimeHandle *, underlying);
  return (size_t)qhpi_num_slices(qhpi);
}

/// @brief retrieves the base address of the current thread runtime
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) ripple_thd_block_t
_ripple_get_thd_block(void *underlying, unsigned block_id) {
  assert(block_id == 0 &&
         "Only one ripple_thd_block supported by this runtime");
  QHPI_RuntimeHandle *qhpi = RTHD_RCAST(QHPI_RuntimeHandle *, underlying);
  return RTHD_RCAST(ripple_thd_block_t, qhpi_sync_block(qhpi));
}

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) ripple_thd_block_t
_ripple_init_thd_block(void *underlying, unsigned block_id) {
  assert(block_id == 0 &&
         "Only one ripple_thd_block supported by this runtime");
  return _ripple_get_thd_block(underlying, block_id);
}

/// Cleaning up is also not really necessary here.
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_clear_thd_block(void *underlying, unsigned block_id) {}

// ________________________________ Barrier interface __________________________

#ifdef __cplusplus
extern "C" {
#endif

/// @brief sets all the barriers to 0.
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_thd_barrier_init(ripple_thd_barrier_t *barriers, size_t barrier_id,
                         size_t n_participants) {
  ripple_thd_barrier_t *barrier = &(barriers[barrier_id]);
  barrier->n_participants = n_participants;
  barrier->count = n_participants;
  barrier->epoch = 0;
}

/// @brief Destroys all the barriers that have been initialized
RIPPLE_THD_SCALAR
inline void _ripple_thd_barrier_exit(ripple_thd_barrier_t *barrier,
                                     size_t bar_id) {}
#ifdef __cplusplus
}
#endif

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) ripple_thd_barrier_t *
_ripple_thd_barriers_alloc(void *underlying, unsigned n_thd,
                           unsigned max_dims) {
  assert(max_dims <= 3 && "QHPI Ripple runtime supports up to 3 thd dims");
  ripple_qhpi_sync_block_t *sync = sync_block_addr(underlying);
  // barriers were allocated in ripple_thd_init. Just return the ptr
  // Note: barriers are shared across blocks.
  return sync->ripple_rt[0]->barriers;
}

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_thd_barriers_dealloc(ripple_thd_barrier_t *base) {
  // No need to do anything with the barriers. They'll get deallocated
  // implicitly
}

/// @brief Spin barrier with a short (32ns) wait between spins
/// @param b A ripple block (pointer)
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_thd_barrier(ripple_thd_barrier_t *base, unsigned idx) {
  ripple_thd_barrier_t *bar = &(base[idx]);
  // Capture the phase we expect to see toggled when the barrier releases.
  uint32_t local_epoch = __atomic_load_n(&(bar->epoch), memory_order_acquire);

  // Decrement the count; fetch_sub returns the previous value.
  int32_t prev = __atomic_fetch_sub(&(bar->count), 1, memory_order_acq_rel);
  assert(prev > 0 && "More thread joining the barrier than n_participants");

  local_epoch++;
  if (prev == 1) {
    // We are the last thread to arrive:
    // 1) Reset the counter for the next use of the barrier.
    // 2) Toggle the epoch to release waiters.
    // Use release store so that all prior writes in this epoch
    // become visible to threads in the next epoch.
    __atomic_store_n(&(bar->count), bar->n_participants, memory_order_release);
    __atomic_thread_fence(memory_order_release);
    // Toggle epoch
    __atomic_store_n(&(bar->epoch), local_epoch, memory_order_release);
  } else {
    // Not the last thread: spin until epoch changes.
    // Use acquire loads so that we observe all writes
    // that happened-before the barrier (enforced by last thread’s release).
    // Start by spinning, then sleeping, then yielding
#ifdef RIPPLE_THD_QHPI_DEBUG
    unsigned n_spins = 0;
#endif
    while (__atomic_load_n(&(bar->epoch), memory_order_acquire) !=
           local_epoch) {
#ifdef RIPPLE_THD_QHPI_DEBUG
      n_spins++;
      if (n_spins == RIPPLE_THD_MAX_SPINS) {
        qurt_printf("[Warning] Thread spun %u times on barrier\n",
                    RIPPLE_THD_MAX_SPINS);
      }
#endif
    }
#if 0 // We'll consider sleeps and yields later
    if (n_spins == RIPPLE_THD_MAX_SPINS) {
    unsigned n_sleeps = 0;
      while (__atomic_load_n(&(bar->epoch), memory_order_acquire) !=
                 local_epoch &&
             n_sleeps < RIPPLE_THD_MAX_SLEEPS) {
        _ripple_thd_sleep();
        n_sleeps++;
      }
    }
    if (n_sleeps == RIPPLE_THD_MAX_SLEEPS) {
      while (__atomic_load_n(&(bar->epoch), memory_order_acquire) !=
      local_epoch) {
        _ripple_thd_yield();
      }
    }
#endif
  }
  // We've done an acquire in all cases, so no memory operation from after the
  // barrier can move before it.
}

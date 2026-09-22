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
#include <ripple/hexagon.h>
#include <ripple/thread.h>

#ifdef __NO_QURT__
int qurt_printf(const char *msg, ...);
qurt_thread_t qurt_thread_get_id(void);
#else
#include <qurt.h>
#endif

#ifdef RIPPLE_THD_QHPI_DEBUG
#define print(x, ...) qurt_printf(x, __VA_ARGS__)
#else
#define print(x, ...)
#endif

/// @brief Retrieves the base address of block # @p block_id
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) static qhpi_runtime_t *
sync_block_addr(void *underlying) {
  QHPI_RuntimeHandle *qhpi = (QHPI_RuntimeHandle *)underlying;
  return (qhpi_runtime_t *)qhpi_sync_block(qhpi);
}

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) static unsigned
ripple_qhpi_n_slices(void *underlying) {
  QHPI_RuntimeHandle *qhpi = (QHPI_RuntimeHandle *)underlying;
  return qhpi_num_slices(qhpi);
}

// _________________________ Underlying runtime ________________________________

#define RIPPLE_THD_PREPARING RIPPLE_THD_MAX_U
#define ADD_BYTES(addr, bytes, type) (type)(((char *)(addr)) + (bytes))

// _ _ _ _ _ _ _ _ Binding qhpi_runtime_t with slice ids _ _ _ _ _ _ _ _ _ _ _ _

// 1. Consistency discussion:
// I think that these routines don't need atomics because:
// - Each thread writes all and only the memory elements that will match
// its search, and from which it will take the QHPI_RuntimeHandle*
// (and deduce its slice index)

// 2. Preemption discussion:
// The functions in this section assume that the _set_ of qurt_thread_get_id's
// that work on the currently active QHPI slices remain unchanged through
// preemption. Users must assume that the current work can be picked up by a
// thread associated with a different ripple_thd_id(). It's ok if the
// association between kernel instance and qurt_thread_get_id changes through
// preemption. But there can't be a new qurt_thread_get_id involved.

/// Retrieves the current thread's slice id
static __attribute__((always_inline)) int _cur_slice_id(qhpi_runtime_t *sync) {
  int n = sync->n_slices;
  int slice_id = 0;
  unsigned thd_id = qurt_thread_get_id();
  for (; slice_id < n; ++slice_id) {
    if (sync->qhpi_pairs[slice_id].thd_id == thd_id)
      return slice_id;
  }
  assert(0 && "Internal error: couln't find my slice id");
  return -1;
}

RIPPLE_THD_SCALAR
static __attribute__((always_inline)) void
_save_qhpi_binding(qhpi_runtime_t *sync, QHPI_RuntimeHandle *qhpi) {
  unsigned slice_id = qhpi_slice_number(qhpi);
  qhpi_thd_pair *cur_pair = &(sync->qhpi_pairs[slice_id]);
  cur_pair->thd_id = qurt_thread_get_id();
  cur_pair->qhpi = qhpi;
}

RIPPLE_THD_SCALAR
static __attribute__((always_inline)) void
_clear_qhpi_binding(qhpi_runtime_t *sync) {
  unsigned slice_id = _cur_slice_id(sync);
  qhpi_thd_pair *cur_pair = &(sync->qhpi_pairs[slice_id]);
  cur_pair->thd_id = 0;
  cur_pair->qhpi = NULL;
}

// _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

// Sync block mem layout:
// | sync_block | qhpi_pairs | ripple_thread_blocks | it_servs | barriers |

RIPPLE_THD_SCALAR
extern size_t ripple_qhpi_runtime_size(unsigned n_slices, unsigned max_dims,
                                       unsigned n_blocks, unsigned flags) {
  size_t sz = sizeof(qhpi_runtime_t) + (sizeof(qhpi_thd_pair) * n_slices);
  sz += sizeof(struct ripple_thread_block) * n_blocks;
  // OPT: there may be cases where we can reduce these sizes to using n_thd,
  // the number of HW threads available.
  if (flags & RIPPLE_THD_USE_DYN) {
    if (max_dims == 1)
      sz += n_blocks * sizeof(it_serv_t);
    else
      sz += n_blocks * ((n_slices + 1) / 2) * sizeof(it_serv_t);
  }
  if (flags & RIPPLE_THD_USE_BAR) {
    sz += n_bars_to_alloc(max_dims, n_slices) * _ripple_barrier_size();
  }
  return sz;
}

/// Initializes ripple_thd's underlying runtime object from the QHPI_Runtime_t *
// handle. Everything but the barrier objects, because their initialization
/// depends upon the thread block shape.
/// @param underlying is the qhpi_runtime_t runtime object
/// @return a pointer to the qhpi_runtime_t underlying runtime object to use
/// with the Ripple thread API.
RIPPLE_THD_SCALAR extern void *ripple_thd_init(int PE_id, void *qhpi,
                                               unsigned n_blocks,
                                               unsigned flags,
                                               unsigned max_dims) {
  assert(qhpi != NULL && "underlying QHPI object has to be non-NULL");
  assert(n_blocks <= RIPPLE_THD_QHPI_MAX_BLOCKS &&
         "Requesting more thread blocks than supported by Ripple-QHPI");
  //       Blocks can share barriers but not iteration servers.
  qhpi_runtime_t *sync = sync_block_addr(qhpi);
  unsigned n_slices = ripple_qhpi_n_slices(qhpi);
  assert(sync != NULL &&
         "Multi-threaded Ripple requires a QHPI sync block. "
         "ripple_qhpi_runtime_size() reports the necessary size\n");
  // First thread to reach the call to ripple_thd_init init's the runtime
  // We assume that the first element of the sync zone was cleared (to zero)
  unsigned *lock = &(sync->lock);
  unsigned zero = 0;
  if (__atomic_compare_exchange_n(lock, &zero, RIPPLE_THD_PREPARING,
                                  /*weak*/ 0, memory_order_acq_rel,
                                  memory_order_acquire)) {
    sync->n_slices = n_slices;
    sync->n_blocks = n_blocks;

    // FIXME: QHPI dropped the num_threads() API, which defined the number
    // of threads associated with a self-threaded Op.
    // As a fix, we're using the underlying QuRT APi to know how many HW
    // threads are in the system, which may be more than QHPI's num_threads,
    // if the system talking to QHPI allocates fewer threads to the op.
    sync->n_threads = ripple_num_hvx();

    sync->qhpi_pairs = ADD_BYTES(sync, sizeof(qhpi_runtime_t), qhpi_thd_pair *);
    // Set the first block address,
    // which will be the base address for the thread blocks.
    sync->ripple_rts = ADD_BYTES(
        sync->qhpi_pairs, sizeof(qhpi_thd_pair) * n_slices, ripple_thd_block_t);
    _save_qhpi_binding(sync, qhpi);

    if ((flags & RIPPLE_THD_USE_BAR) && n_slices > sync->n_threads) {
      assert(0 &&
             "[Error] ripple_thd_barrier() cannot be used with more slices"
             " than HW threads --> deadlock\n"
             "If you aren't using barriers,"
             " don't pass the RIPPLE_THD_USE_BAR flag to ripple_thd_init()");
    }
    _ripple_thd_init(PE_id, sync, n_blocks, n_slices, flags, max_dims);
    __atomic_thread_fence(memory_order_release);
    __atomic_store_n(lock, 1, memory_order_release);
    return sync;
  } else {
    // Wait for the runtime to be initialized
    while (__atomic_load_n(lock, memory_order_acquire) == RIPPLE_THD_PREPARING)
      ;
    _save_qhpi_binding(sync, qhpi);
    return sync;
  }
}

#undef RIPPLE_THD_PREPARING

RIPPLE_THD_SCALAR
extern void ripple_thd_exit(void *qhpi) {
  assert(qhpi != NULL && "underlying runtime object has to be non-NULL");
  qhpi_runtime_t *sync = sync_block_addr(qhpi);
  _clear_qhpi_binding(sync);
}

// ______________________________ Core API Helpers _____________________________

/// @brief Coordinate of the current thread within the block along dimension
/// dim
RIPPLE_THD_SCALAR
extern __attribute__((always_inline)) size_t
_ripple_thd_flat_id(void *underlying) {
  qhpi_runtime_t *sync = (qhpi_runtime_t *)underlying;
  return (size_t)_cur_slice_id(sync);
}

/// @brief full number of threads available from the runtime.
RIPPLE_THD_SCALAR
extern __attribute__((always_inline)) size_t
_ripple_thd_flat_shape(void *underlying) {
  qhpi_runtime_t *sync = (qhpi_runtime_t *)underlying;
  return sync->n_slices;
}

/// @brief retrieves the base address of the current thread runtime
RIPPLE_THD_SCALAR
extern __attribute__((always_inline)) ripple_thd_block_t
_ripple_get_thd_block(void *underlying, unsigned block_id) {
  assert(underlying != NULL);
  qhpi_runtime_t *sync = (qhpi_runtime_t *)underlying;
  return ADD_BYTES(sync->ripple_rts,
                   block_id * sizeof(struct ripple_thread_block),
                   ripple_thd_block_t);
}

/// Cleaning up is also not really necessary here.
RIPPLE_THD_SCALAR
extern __attribute__((always_inline)) void
_ripple_clear_thd_block(void *sync, unsigned block_id) {}

// ________________________________ Barrier interface __________________________

#ifdef __cplusplus
extern "C" {
#endif

/// @brief initiaLizes barrier @p barrier_id to 0.
RIPPLE_THD_SCALAR
extern __attribute__((always_inline)) void
_ripple_thd_barrier_init(ripple_thd_barrier_t *barriers, size_t barrier_id,
                         size_t n_participants) {
  ripple_thd_barrier_t *barrier = &(barriers[barrier_id]);
  barrier->n_participants = n_participants;
  barrier->count = n_participants;
  barrier->epoch = 0;
}

/// @brief Destroys all the barriers that have been initialized
RIPPLE_THD_SCALAR
extern void _ripple_thd_barrier_exit(ripple_thd_barrier_t *barrier,
                                     size_t bar_id) {}
#ifdef __cplusplus
}
#endif

RIPPLE_THD_SCALAR
extern size_t _ripple_barrier_size() { return sizeof(ripple_thd_barrier_t); }

/// @brief Spin barrier with a short (32ns) wait between spins
/// @param b A ripple block (pointer)
RIPPLE_THD_SCALAR inline __attribute__((always_inline)) void
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

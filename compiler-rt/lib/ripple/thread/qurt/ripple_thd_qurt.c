//====------------------------ ripple_thd_qurt.cpp ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// QuRT-specific Ripple thread implementation
//
//===----------------------------------------------------------------------===//

#include "ripple/ripple_thd_qurt.h"
#include "qurt.h"
#include <stdint.h>
#include <stdlib.h>

// _________________________ Underlying runtime ________________________________

qthd_runtime_t *qthd_runtime_init(unsigned n_thd_) {
  qthd_runtime_t *rt = (qthd_runtime_t *)malloc(sizeof(qthd_runtime_t));
  unsigned max_n_thd = qurt_hvx_get_units();
  if (n_thd_ > max_n_thd) {
    qurt_printf("[Note] Capping # qthd threads to %u\n", max_n_thd);
    n_thd_ = max_n_thd;
  }
  rt->n_thd = n_thd_;
  rt->threads = (qurt_thread_t *)malloc(n_thd_ * sizeof(qurt_thread_t));
  rt->attrs = (qurt_thread_attr_t *)malloc(n_thd_ * sizeof(qurt_thread_attr_t));
  rt->ctxs = (context_f_args_t *)malloc(n_thd_ * sizeof(context_f_args_t));
  rt->stack = (char **)malloc(n_thd_ * sizeof(char *));

  for (unsigned t = 0; t < n_thd_; ++t) {
    // Generate mini-names "0", "1", etc, which we'll convert back to ints
    // to use as thread ids.
    _qurt_ripple_thd_write_id(rt->names + QURT_RIPPLE_MAX_THD_NAME_SIZE * t, t);
    // stacks must be 8-byte aligned
    rt->stack[t] = (char *)aligned_alloc(8, RIPPLE_THD_QURT_STACK_SIZE);
  }
  rt->return_status = (int *)malloc(n_thd_ * sizeof(int));
  rt->ripple_rt = NULL;
  return rt;
}

void qthd_runtime_exit(qthd_runtime_t *rt) {
  unsigned n_thd = rt->n_thd;
  free(rt->threads);
  free(rt->attrs);
  free(rt->ctxs);
  for (unsigned t = 0; t < n_thd; ++t) {
    free(rt->stack[t]);
  }
  free(rt->stack);
  free(rt->return_status);
  rt->ripple_rt = NULL;
  free(rt);
}

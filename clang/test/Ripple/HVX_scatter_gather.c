// XFAIL: *
// REQUIRES: hexagon-registered-target
// RUN: %clang -g -S -fenable-ripple --target=hexagon -emit-llvm %s -o - -mllvm -ripple-disable-link  2>&1 | FileCheck %s

#include "ripple_test.h"
// HVX_scatter_gather RT library declarations, inlined so this frontend test
// does not depend on the compiler-rt headers or the Hexagon SDK.

void HVX_scatter_16(uint16_t *destination, uint16_t *source, uint16_t *indexes,
                    uint32_t region_len);
void HVX_scatter_acc_16(uint16_t *destination, uint16_t *source,
                        uint16_t *indexes, uint32_t region_len);
void HVX_scatter_q_16(uint16_t *predicates, uint16_t *destination,
                      uint16_t *source, uint16_t *indexes,
                      uint32_t region_len);
void HVX_scatter_32(uint32_t *destination, uint32_t *source, uint32_t *indexes,
                    uint32_t vector_size, uint32_t region_len);
void HVX_scatter_acc_32(uint32_t *destination, uint32_t *source,
                        uint32_t *indexes, uint32_t vector_size,
                        uint32_t region_len);
void HVX_scatter_q_32(uint32_t *predicates, uint32_t *destination,
                      uint32_t *source, uint32_t *indexes, uint32_t vector_size,
                      uint32_t region_len);
void HVX_scatter_16_32(uint16_t *destination, uint16_t *source,
                       uint32_t *indexes, uint32_t region_len);
void HVX_scatter_acc_16_32(uint16_t *destination, uint16_t *source,
                           uint32_t *indexes, uint32_t region_len);
void HVX_scatter_q_16_32(uint16_t *predicates, uint16_t *destination,
                         uint16_t *source, uint32_t *indexes,
                         uint32_t region_len);
void HVX_gather_16(uint16_t *destination, uint16_t *source, uint16_t *indexes,
                   uint32_t region_len);
void HVX_gather_q_16(uint16_t *predicates, uint16_t *destination,
                     uint16_t *source, uint16_t *indexes, uint32_t region_len);
void HVX_gather_32(uint32_t *destination, uint32_t *source, uint32_t *indexes,
                   uint32_t vector_size, uint32_t region_len);
void HVX_gather_q_32(uint32_t *predicates, uint32_t *destination,
                     uint32_t *source, uint32_t *indexes, uint32_t vector_size,
                     uint32_t region_len);
void HVX_gather_16_32(uint16_t *destination, uint16_t *source,
                      uint32_t *indexes, uint32_t region_len);
void HVX_gather_q_16_32(uint16_t *predicates, uint16_t *destination,
                        uint16_t *source, uint32_t *indexes,
                        uint32_t region_len);

#define HVX_32_i32 32
__attribute__((noinline)) void Ripple_scatter_32_library(unsigned int *destination, unsigned int *source, unsigned int *indexes)  {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  size_t v = ripple_id(BS, 0);
  int _ripple_block_0;
  int _ripple_ub_0 = ((HVX_32_i32 - 0) / ripple_get_block_size(BS, 0));
  for (_ripple_block_0 = 0; _ripple_block_0 < _ripple_ub_0; _ripple_block_0 += 1) {
    int i = ripple_get_block_size(BS, 0) * _ripple_block_0 + 0 + ripple_id(BS, 0);
    HVX_scatter_32(destination, source, indexes, 64, 8192);
  }
}
// CHECK: @Ripple_scatter_32_library
// CHECK: tail call void @ripple_HVX_scatter_32
// CHECK: ret void

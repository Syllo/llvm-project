// REQUIRES: hexagon-registered-target
// RUN: %clang --target=hexagon-unknown-elf -g -c -O2 -emit-llvm %S/external_library.c -o %t.rlib.bc
// RUN: %clang --target=hexagon-unknown-elf -c -O2 -fno-discard-value-names -fenable-ripple -emit-llvm -S -o - -fripple-lib %t.rlib.bc -mllvm -ripple-disable-link %s | FileCheck %s

#include "external_library.h"
#include <stddef.h>
#include <ripple.h>

#define VEC 0

// CHECK: ew_smaller
// CHECK: for.end
// CHECK: %[[Load:[a-zA-Z0-9_.]+]] ={{.*}}<16 x half>{{.*}}load{{.*}}
// CHECK-NEXT: %[[Shuffle1:[a-zA-Z0-9_.]+]] = shufflevector <16 x half> %[[Load]]
// CHECK-NEXT: %[[Shuffle2:[a-zA-Z0-9_.]+]] = shufflevector <32 x half> %[[Shuffle1]]
// CHECK-NEXT: call void @ripple_ew_pure_cosf16(ptr nonnull %[[RetBuffer:[a-zA-Z0-9_.]+]], <64 x half> %[[Shuffle2]])
// CHECK-NEXT: %[[LoadRet:[a-zA-Z0-9_.]+]] = load <64 x half>, ptr %[[RetBuffer]]
// CHECK-NEXT: %[[Extract:[a-zA-Z0-9_.]+]] = shufflevector <64 x half> %[[LoadRet]]
// CHECK: store{{.*}}<16 x half> %[[Extract]]

void ew_smaller(size_t size, _Float16 *input, _Float16 *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 16);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  size_t i;
  for (i = 0; i + BlockSizeX < size; i += BlockSizeX)
    output[i + BlockX] = cosf16(input[i + BlockX]);
  if(i + BlockX < size)
    output[i + BlockX] = cosf16(input[i + BlockX]);
}

// CHECK: ew_sizematch
// CHECK: for.end
// CHECK: %[[Load:[a-zA-Z0-9_.]+]] ={{.*}}<64 x half>{{.*}}load{{.*}}
// CHECK-NEXT: call void @ripple_ew_pure_cosf16(ptr nonnull %[[RetBuffer:[a-zA-Z0-9_.]+]], <64 x half> %[[Load]])
// CHECK-NEXT: %[[LoadRet:[a-zA-Z0-9_.]+]] = load <64 x half>, ptr %[[RetBuffer]]
// CHECK: store{{.*}}<64 x half> %[[LoadRet]]

void ew_sizematch(size_t size, _Float16 *input, _Float16 *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 64);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  size_t i;
  for (i = 0; i + BlockSizeX < size; i += BlockSizeX)
    output[i + BlockX] = cosf16(input[i + BlockX]);
  if(i + BlockX < size)
    output[i + BlockX] = cosf16(input[i + BlockX]);
}

// CHECK: ew_larger
// CHECK: for.end
// CHECK: %[[Load:[a-zA-Z0-9_.]+]] ={{.*}}<138 x half>{{.*}}load{{.*}}
// CHECK-NEXT: %[[Slice1:[a-zA-Z0-9_.]+]] = shufflevector <138 x half> %[[Load]]
// CHECK-NEXT: call void @ripple_ew_pure_cosf16(ptr nonnull %[[RetBuffer:[a-zA-Z0-9_.]+]], <64 x half> %[[Slice1]])
// CHECK-NEXT: %[[LoadRet1:[a-zA-Z0-9_.]+]] = load <64 x half>, ptr %[[RetBuffer]]
// CHECK-NEXT: %[[Slice2:[a-zA-Z0-9_.]+]] = shufflevector <138 x half> %[[Load]]
// CHECK-NEXT: call void @ripple_ew_pure_cosf16(ptr nonnull %[[RetBuffer:[a-zA-Z0-9_.]+]], <64 x half> %[[Slice2]])
// CHECK-NEXT: %[[LoadRet2:[a-zA-Z0-9_.]+]] = load <64 x half>, ptr %[[RetBuffer]]
// CHECK-NEXT: %[[Slice3:[a-zA-Z0-9_.]+]] = shufflevector <138 x half> %[[Load]]
// CHECK-NEXT: call void @ripple_ew_pure_cosf16(ptr nonnull %[[RetBuffer:[a-zA-Z0-9_.]+]], <64 x half> %[[Slice3]])
// CHECK-NEXT: %[[LoadRet3:[a-zA-Z0-9_.]+]] = load <64 x half>, ptr %[[RetBuffer]]
// CHECK-NEXT: %[[Fuse1:[a-zA-Z0-9_.]+]] = shufflevector <64 x half> %[[LoadRet1]], <64 x half> %[[LoadRet2]]
// CHECK-NEXT: %[[Fuse2:[a-zA-Z0-9_.]+]] = shufflevector <64 x half> %[[LoadRet3]]
// CHECK-NEXT: %[[FuseFinal:[a-zA-Z0-9_.]+]] = shufflevector <128 x half> %[[Fuse1]], <128 x half> %[[Fuse2]]
// CHECK: store{{.*}}<138 x half> %[[FuseFinal]]

void ew_larger(size_t size, _Float16 *input, _Float16 *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 138);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  size_t i;
  for (i = 0; i + BlockSizeX < size; i += BlockSizeX)
    output[i + BlockX] = cosf16(input[i + BlockX]);
  if(i + BlockX < size)
    output[i + BlockX] = cosf16(input[i + BlockX]);
}


// CHECK: ew_multidim
// CHECK: for.end
// CHECK: %[[Slice1:[a-zA-Z0-9_.]+]] = shufflevector <128 x half> %[[Load:[a-zA-Z0-9_.]+]]
// CHECK-NEXT: call void @ripple_ew_pure_cosf16(ptr nonnull %[[RetBuffer:[a-zA-Z0-9_.]+]], <64 x half> %[[Slice1]])
// CHECK-NEXT: %[[LoadRet1:[a-zA-Z0-9_.]+]] = load <64 x half>, ptr %[[RetBuffer]]
// CHECK-NEXT: %[[Slice2:[a-zA-Z0-9_.]+]] = shufflevector <128 x half> %[[Load]]
// CHECK-NEXT: call void @ripple_ew_pure_cosf16(ptr nonnull %[[RetBuffer:[a-zA-Z0-9_.]+]], <64 x half> %[[Slice2]])
// CHECK-NEXT: %[[LoadRet2:[a-zA-Z0-9_.]+]] = load <64 x half>, ptr %[[RetBuffer]]
// CHECK-NEXT: %[[FuseFinal:[a-zA-Z0-9_.]+]] = shufflevector <64 x half> %[[LoadRet1]], <64 x half> %[[LoadRet2]]

void ew_multidim(size_t size, _Float16 *input, _Float16 *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 4, 32);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockY = ripple_id(BS, 1);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  size_t BlockSizeY = ripple_get_block_size(BS, 1);
  size_t i;
  for (i = 0; i + BlockSizeX + BlockSizeY < size; i += BlockSizeX + BlockSizeY)
    output[i + BlockX + BlockY] = cosf16(input[i + BlockX + BlockY]);
  if (i + BlockX + BlockY < size)
    output[i + BlockX + BlockY] = cosf16(input[i + BlockX + BlockY]);
}

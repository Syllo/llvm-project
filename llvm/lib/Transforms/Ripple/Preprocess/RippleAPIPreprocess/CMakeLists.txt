add_llvm_component_library(LLVMRippleAPIPreprocess
  RippleAPIPreprocess.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/Ripple

  DEPENDS
  intrinsics_gen

  LINK_COMPONENTS
  Analysis
  Core
  Ripple
  Support
  TransformUtils
)

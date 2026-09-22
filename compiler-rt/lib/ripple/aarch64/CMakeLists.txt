cmake_minimum_required(VERSION 3.20.0)
project(RippleSMEVectorLibrary C CXX)

set(COMPILER_RT_DEFAULT_TARGET_TRIPLE
    "${COMPILER_RT_DEFAULT_TARGET_TRIPLE}"
    CACHE STRING "Target triple (e.g., aarch64-linux-gnu)")
set(RIPPLE_SUPPORTED_ARCH aarch64)

# Set target architecture
set(_derived_arch "")
# Extract first token (arch) from triple and normalize aliases
if(COMPILER_RT_DEFAULT_TARGET_TRIPLE)
  # Split triple by '-': "<arch>-<vendor>-<os>-<abi>"
  string(REPLACE "-" ";" _triple_parts "${COMPILER_RT_DEFAULT_TARGET_TRIPLE}")
  list(GET _triple_parts 0 _arch_raw)
  string(TOLOWER "${_arch_raw}" _arch)

  # Normalize common aliases to canonical names
  if(_arch STREQUAL "arm64")      # Apple alias for aarch64
    set(_arch "aarch64")
  elseif(_arch STREQUAL "armv8")  # Often equivalent to aarch64
    set(_arch "aarch64")
  endif()

  set(_derived_arch "${_arch}")
endif()

# Check if target is supported
if(NOT _derived_arch)
  message(WARNING
    "Ripple aarch64 vector library disabled: no target triple provided. "
    "Pass -DCOMPILER_RT_DEFAULT_TARGET_TRIPLE=aarch64-linux-gnu to enable.")
  return()
elseif(NOT "${_derived_arch}" IN_LIST RIPPLE_SUPPORTED_ARCH)
  message(WARNING
    "Ripple aarch64 vector library disabled. "
    "Target ${_derived_arch} not supported.")
  return()
else()
  message(STATUS "Installing Ripple SME runtime headers for ${_derived_arch}")
endif()

set(RIPPLE_RT_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/header)

# Keep standalone configuration of this directory working too.
if(NOT DEFINED COMPILER_RT_INSTALL_INCLUDE_DIR)
  set(COMPILER_RT_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()

# Preserve the public <ripple_sme_veclib.h> include spelling.
install(
  FILES ${RIPPLE_RT_HEADER_DIR}/ripple_sme_veclib.h
  COMPONENT compiler-rt-headers
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${COMPILER_RT_INSTALL_INCLUDE_DIR}
)

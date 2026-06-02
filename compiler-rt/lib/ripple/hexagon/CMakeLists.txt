if(NOT "${COMPILER_RT_DEFAULT_TARGET_ARCH}" IN_LIST RIPPLE_SUPPORTED_ARCH)
  message(
    WARNING
      "--> Current compiler RT target (${COMPILER_RT_DEFAULT_TARGET_ARCH}) is not supported for Ripple RT libs."
  )
  return()
endif()

message(STATUS "--> Installing Ripple RT headers (header-only library)")

set(RIPPLE_RT_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/header)

file(GLOB RIPPLE_RT_HEADERS ${RIPPLE_RT_HEADER_DIR}/*.h)
install(
  FILES ${RIPPLE_RT_HEADERS}
  COMPONENT compiler-rt-headers
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION ${COMPILER_RT_INSTALL_INCLUDE_DIR}/ripple)

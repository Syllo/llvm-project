
# __________________________ Common ripple thd files ___________________________

get_compiler_rt_install_dir(${COMPILER_RT_DEFAULT_TARGET_ARCH} install_dir)

message(NOTICE "RIPPLE THD INSTALL DIR is ${install_dir}")

set (COMMON_HEXAGON_HEADERS 
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_block.h
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_conventions.h
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/hexagon.h
)

# Bail if the target architecture doesn't have compiler-rt code to suppor Ripple
if(NOT "${COMPILER_RT_DEFAULT_TARGET_ARCH}" IN_LIST RIPPLE_SUPPORTED_ARCH)
  message(
    STATUS
      "Ripple Multi-threading not supported for ${COMPILER_RT_DEFAULT_TARGET_ARCH})"
  )
  return()
endif()

# __________________________ Environment detection _____________________________

function(set_hexagon_version_info HEXAGON_ARCH_VERSION EXTRA_HEXAGON_FLAGS_OUT QURT_COMPUTE_PATH_OUT)
  # HEXAGON ARCH VERSION FORMAT:
  # v##HEXAGON_MAJOR_VERSION##_##HEXAGON_MINOR_VERSION##
  string(REGEX MATCH "^v([0-9]+)t?(_([0-9]+))?" HEXAGON_VERSION_MATCHED
         "${HEXAGON_ARCH_VERSION}")
  if(NOT HEXAGON_VERSION_MATCHED)
    message(FATAL_ERROR "Invalid HEXAGON_ARCH_VERSION: ${HEXAGON_ARCH_VERSION}")
  endif()

  set(_hexagon_major_version "${CMAKE_MATCH_1}")

  set(_extra_hexagon_flags
      -DHEXAGON_MAJOR_VERSION=${_hexagon_major_version})

  if(CMAKE_MATCH_3)
    set(HEXAGON_MINOR_VERSION "${CMAKE_MATCH_3}")
    list(APPEND _extra_hexagon_flags
         -DHEXAGON_MINOR_VERSION=${HEXAGON_MINOR_VERSION})
  else()
    set(HEXAGON_MINOR_VERSION 0)
    list(APPEND _extra_hexagon_flags
         -DHEXAGON_MINOR_VERSION=0)
  endif()

  set(${EXTRA_HEXAGON_FLAGS_OUT} ${_extra_hexagon_flags} PARENT_SCOPE)

  set(_qurt_candidates
  "computev${_hexagon_major_version}"
  "computev81"
  "computev79"
  "computev75"
  "computev69"
  )

  foreach(_candidate IN LISTS _qurt_candidates)
    set(_candidate_dir
      "${HEXAGON_SDK_ROOT}/rtos/qurt/${_candidate}")
    if(IS_DIRECTORY "${_candidate_dir}")
      set(_qurt_compute_path "${_candidate}")
      break()
    endif()
  endforeach()

  set(${QURT_COMPUTE_PATH_OUT}
      ${_qurt_compute_path}
      PARENT_SCOPE)
endfunction()

# Detect QuRT
if ("${HEXAGON_SDK_ROOT}" STREQUAL "")
  set(HEXAGON_SDK_ROOT "$ENV{HEXAGON_SDK_ROOT}")
endif()
if(NOT "${HEXAGON_SDK_ROOT}" STREQUAL "")
  message(STATUS "Using HEXAGON_SDK_ROOT: ${HEXAGON_SDK_ROOT}")
  if(NOT "${HEXAGON_ARCH_VERSION}" STREQUAL "")
    message(STATUS "Target HEXAGON_ARCH_VERSION: ${HEXAGON_ARCH_VERSION}")
    set_hexagon_version_info("${HEXAGON_ARCH_VERSION}" HEXAGON_ARCH_DEFS QURT_COMPUTE_PATH)
  endif()
endif()

if ("${QURT_COMPUTE_PATH}" STREQUAL "")
  message(STATUS "QuRT not found - building without it")
  set (HAS_QURT "FALSE")
else()
  message(STATUS "Found QuRT at ${QURT_COMPUTE_PATH}")
  set(HEXAGON_QURT_INCLUDE_DIR "${HEXAGON_SDK_ROOT}/rtos/qurt/${QURT_COMPUTE_PATH}/include/qurt"
    CACHE PATH "QuRT include dir"
    FORCE
  )
  set (HAS_QURT "TRUE")
endif()

# Detect the tool set. Several variables can influence this.
# Preferred: find a toolset in which the compiler used to build compiler-rt sits
if (NOT "${CMAKE_C_COMPILER}" STREQUAL "")
  # Expected structure: HEXAGON_TOOLS_ROOT/bin/clang
  message(NOTICE "Picking up tools from c compiler ${CMAKE_C_COMPILER}")
  get_filename_component(_clang_parent "${CMAKE_C_COMPILER}" DIRECTORY)
else()
  if (NOT "${CMAKE_CXX_COMPILER}" STREQUAL "")
    get_filename_component(_clang_parent "${CMAKE_CXX_COMPILER}" DIRECTORY)
  endif()
endif()
if (NOT "${_clang_parent}" STREQUAL "")
  get_filename_component(_tools_base "${_clang_parent}" DIRECTORY)
  message(NOTICE "Looking for a toolset at ${_tools_base}")
  # We need the target/ subdirectory to be there in order to use the toolset
  set (_toolset_target_dir "${_tools_base}/target")
  if (IS_DIRECTORY "${_toolset_target_dir}")
    message(STATUS "Found toolset at ${_tools_base}")
    set (HEXAGON_TOOLS_ROOT "${_tools_base}")
  else()
    message(NOTICE "target dir not found at ${_toolset_target_dir}")
  endif()
endif()

if ("${HEXAGON_TOOLS_ROOT}" STREQUAL "")
    message(FATAL_ERROR "Hexagon tools root folder not found. "
    "Did you setup the SDK environment variables (setup_sdk_env.source) ?")
endif()

set(RIPPLE_THD_HEXAGON_FLAGS
    -O2
    -Wall
    -m${HEXAGON_ARCH_VERSION}
#    ${HEXAGON_ARCH_DEFS}
    -mhvx
    -Wno-gnu-include-next
    -fenable-ripple
    )

if (NOT "${HAS_QURT}" STREQUAL "TRUE")
  list(APPEND RIPPLE_THD_HEXAGON_FLAGS "-D__NO_QURT__")
endif()

# __________________________ QuRT-based thd lib ________________________________

# Sources
set(RIPPLE_THD_QURT_SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/qurt/ripple_thd_qurt.c"
  "${CMAKE_CURRENT_LIST_DIR}/ripple_thd_it_serv.c"
  "${CMAKE_CURRENT_LIST_DIR}/ripple_thd.c"
)

# Include dirs needed to compile the sources
 set(RIPPLE_THD_QURT_INCLUDE_DIRS
  "${HEXAGON_TOOLS_ROOT}/target/hexagon/include"
  "${CMAKE_CURRENT_LIST_DIR}/header"
 )

 if ("${HAS_QURT}" STREQUAL "TRUE")
    list(APPEND RIPPLE_THD_QURT_INCLUDE_DIRS
      "${HEXAGON_QURT_INCLUDE_DIR}"
      "${HEXAGON_QURT_INCLUDE_DIR}/../posix"
    )
endif()

# Copy the flags and modify them for the QuRT runtime
set (RIPPLE_THD_QURT_HEXAGON_FLAGS ${RIPPLE_THD_HEXAGON_FLAGS})

# Keep targets that share OUTPUT_NAME in distinct build-tree directories.
# They install to distinct destinations, but CMake/Ninja still needs unique
# build-tree output paths.
set(RIPPLE_THD_QURT_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/qurt")
set(RIPPLE_THD_QURT_G0PIC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/qurt/G0/pic")

# Static library
add_library(ripple_thd_qurt_static STATIC ${RIPPLE_THD_QURT_SOURCES})
target_include_directories(ripple_thd_qurt_static
PRIVATE
  ${RIPPLE_THD_QURT_INCLUDE_DIRS}
)
target_compile_options(ripple_thd_qurt_static
PRIVATE
  ${RIPPLE_THD_QURT_HEXAGON_FLAGS}
)
set_target_properties(ripple_thd_qurt_static PROPERTIES
  OUTPUT_NAME ripple_thd_qurt
  ARCHIVE_OUTPUT_DIRECTORY "${RIPPLE_THD_QURT_OUTPUT_DIR}"
)

# Static library in G0 + pic
add_library(ripple_thd_qurt_static_g0pic STATIC ${RIPPLE_THD_QURT_SOURCES})
target_include_directories(ripple_thd_qurt_static_g0pic
PRIVATE
  ${RIPPLE_THD_QURT_INCLUDE_DIRS}
)
target_compile_options(ripple_thd_qurt_static_g0pic
PRIVATE
  ${RIPPLE_THD_QURT_HEXAGON_FLAGS}
  -fPIC
  -G0
)
set_target_properties(ripple_thd_qurt_static_g0pic PROPERTIES
  OUTPUT_NAME ripple_thd_qurt
  ARCHIVE_OUTPUT_DIRECTORY "${RIPPLE_THD_QURT_G0PIC_OUTPUT_DIR}"
)

# Shared (dynamic) library
add_library(ripple_thd_qurt_shared SHARED ${RIPPLE_THD_QURT_SOURCES})
target_include_directories(ripple_thd_qurt_shared
PRIVATE
  ${RIPPLE_THD_QURT_INCLUDE_DIRS}
)
target_compile_options(ripple_thd_qurt_shared
PRIVATE
  ${RIPPLE_THD_QURT_HEXAGON_FLAGS}
)
set_target_properties(ripple_thd_qurt_shared PROPERTIES
  OUTPUT_NAME ripple_thd_qurt
  LIBRARY_OUTPUT_DIRECTORY "${RIPPLE_THD_QURT_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${RIPPLE_THD_QURT_OUTPUT_DIR}"
)

# Shared (dynamic) library in G0 + pic
add_library(ripple_thd_qurt_shared_g0pic SHARED ${RIPPLE_THD_QURT_SOURCES})
target_include_directories(ripple_thd_qurt_shared_g0pic
PRIVATE
  ${RIPPLE_THD_QURT_INCLUDE_DIRS}
)
target_compile_options(ripple_thd_qurt_shared_g0pic
PRIVATE
  ${RIPPLE_THD_QURT_HEXAGON_FLAGS}
  -fPIC
  -G0
)
set_target_properties(ripple_thd_qurt_shared_g0pic PROPERTIES
  OUTPUT_NAME ripple_thd_qurt
  LIBRARY_OUTPUT_DIRECTORY "${RIPPLE_THD_QURT_G0PIC_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${RIPPLE_THD_QURT_G0PIC_OUTPUT_DIR}"
)


# Install headers
install(
  FILES
  ${COMMON_HEXAGON_HEADERS}
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_qurt.h
  COMPONENT compiler-rt-headers
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION
  ${COMPILER_RT_INSTALL_INCLUDE_DIR}/ripple
)
install(
  FILES
  ${COMMON_HEXAGON_HEADERS}
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_qurt.h
  COMPONENT compiler-rt-headers
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION
  ${COMPILER_RT_INSTALL_INCLUDE_DIR}/../G0/include/ripple
)

# Install libs
install(TARGETS ripple_thd_qurt_static ripple_thd_qurt_shared
  ARCHIVE DESTINATION ${install_dir}   # static libs (.a/.lib)
  LIBRARY DESTINATION ${install_dir}   # shared libs (.so/.dylib)
  # RUNTIME DESTINATION ${install_dir}   # Windows DLLs
)
# G0 + pic
install(TARGETS ripple_thd_qurt_static_g0pic ripple_thd_qurt_shared_g0pic
  ARCHIVE DESTINATION ${install_dir}/../G0/pic   # static libs (.a/.lib)
  LIBRARY DESTINATION ${install_dir}/../G0/pic   # shared libs (.so/.dylib)
  # RUNTIME DESTINATION ${install_dir}   # Windows DLLs
)

# _ _ _ _ _ _ _ _ _ _ _ _ _  QHPI-based thd libs _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

message(NOTICE "Building QHPI-based Ripple thd lib")

# Sources
set(RIPPLE_THD_QHPI_SOURCES
  "${CMAKE_CURRENT_LIST_DIR}/qhpi/ripple_thd_qhpi.c"
  "${CMAKE_CURRENT_LIST_DIR}/ripple_thd_it_serv.c"
  "${CMAKE_CURRENT_LIST_DIR}/ripple_thd.c"
)

# Include dirs needed to compile the sources
set(RIPPLE_THD_QHPI_INCLUDE_DIRS
  "${HEXAGON_TOOLS_ROOT}/target/hexagon/include"
  "${CMAKE_CURRENT_LIST_DIR}/header"
)

if ("${HAS_QURT}" STREQUAL "TRUE")
  list(APPEND RIPPLE_THD_QHPI_INCLUDE_DIRS
    "${HEXAGON_QURT_INCLUDE_DIR}"
  )
endif()

# Copy the flags and modify them for the QHPI runtime (build w/o qhpi headers)
set (RIPPLE_THD_QHPI_HEXAGON_FLAGS ${RIPPLE_THD_HEXAGON_FLAGS})
list(APPEND RIPPLE_THD_QHPI_HEXAGON_FLAGS "-D__NO_QHPI__")

set(RIPPLE_THD_QHPI_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/qhpi")
set(RIPPLE_THD_QHPI_G0PIC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/qhpi/G0/pic")

# Static library
add_library(ripple_thd_qhpi_static STATIC ${RIPPLE_THD_QHPI_SOURCES})
target_include_directories(ripple_thd_qhpi_static
PRIVATE
  ${RIPPLE_THD_QHPI_INCLUDE_DIRS}
)
target_compile_definitions(ripple_thd_qhpi_static
PRIVATE
  RIPPLE_THD_USE_QHPI
)
target_compile_options(ripple_thd_qhpi_static
PRIVATE
  ${RIPPLE_THD_QHPI_HEXAGON_FLAGS}
)
set_target_properties(ripple_thd_qhpi_static PROPERTIES
  OUTPUT_NAME ripple_thd_qhpi
  ARCHIVE_OUTPUT_DIRECTORY "${RIPPLE_THD_QHPI_OUTPUT_DIR}"
)

# Static library in G0 + pic
add_library(ripple_thd_qhpi_static_g0pic STATIC ${RIPPLE_THD_QHPI_SOURCES})
target_include_directories(ripple_thd_qhpi_static_g0pic
PRIVATE
  ${RIPPLE_THD_QHPI_INCLUDE_DIRS}
)
target_compile_definitions(ripple_thd_qhpi_static_g0pic
PRIVATE
  RIPPLE_THD_USE_QHPI
)
target_compile_options(ripple_thd_qhpi_static_g0pic
PRIVATE
  ${RIPPLE_THD_QHPI_HEXAGON_FLAGS}
  -fPIC
  -G0
)
set_target_properties(ripple_thd_qhpi_static_g0pic PROPERTIES
  OUTPUT_NAME ripple_thd_qhpi
  ARCHIVE_OUTPUT_DIRECTORY "${RIPPLE_THD_QHPI_G0PIC_OUTPUT_DIR}"
)


# Shared (dynamic) library
add_library(ripple_thd_qhpi_shared SHARED ${RIPPLE_THD_QHPI_SOURCES})
target_include_directories(ripple_thd_qhpi_shared
PRIVATE
  ${RIPPLE_THD_QHPI_INCLUDE_DIRS}
)
target_compile_definitions(ripple_thd_qhpi_shared
PRIVATE
  RIPPLE_THD_USE_QHPI
)
target_compile_options(ripple_thd_qhpi_shared
PRIVATE
  ${RIPPLE_THD_QHPI_HEXAGON_FLAGS}
)
set_target_properties(ripple_thd_qhpi_shared PROPERTIES
  OUTPUT_NAME ripple_thd_qhpi
  LIBRARY_OUTPUT_DIRECTORY "${RIPPLE_THD_QHPI_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${RIPPLE_THD_QHPI_OUTPUT_DIR}"
)

# Shared (dynamic) library in G0 + pic
add_library(ripple_thd_qhpi_shared_g0pic SHARED ${RIPPLE_THD_QHPI_SOURCES})
target_include_directories(ripple_thd_qhpi_shared_g0pic
PRIVATE
  ${RIPPLE_THD_QHPI_INCLUDE_DIRS}
)
target_compile_definitions(ripple_thd_qhpi_shared_g0pic
PRIVATE
  RIPPLE_THD_USE_QHPI
)

target_compile_options(ripple_thd_qhpi_shared_g0pic
PRIVATE
  ${RIPPLE_THD_QHPI_HEXAGON_FLAGS}
  -fPIC
  -G0
)
set_target_properties(ripple_thd_qhpi_shared_g0pic PROPERTIES
  OUTPUT_NAME ripple_thd_qhpi
  LIBRARY_OUTPUT_DIRECTORY "${RIPPLE_THD_QHPI_G0PIC_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${RIPPLE_THD_QHPI_G0PIC_OUTPUT_DIR}"
)

# Install headers
install(
  FILES
  ${COMMON_HEXAGON_HEADERS}
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_qhpi.h
  COMPONENT compiler-rt-headers
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION
  ${COMPILER_RT_INSTALL_INCLUDE_DIR}/ripple
)
install(
  FILES
  ${COMMON_HEXAGON_HEADERS}
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_qhpi.h
  COMPONENT compiler-rt-headers
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION
  ${COMPILER_RT_INSTALL_INCLUDE_DIR}/../G0/include/ripple
)


# Install libs
install(TARGETS ripple_thd_qhpi_static ripple_thd_qhpi_shared
  ARCHIVE DESTINATION ${install_dir}   # static libs (.a/.lib)
  LIBRARY DESTINATION ${install_dir}   # shared libs (.so/.dylib)
  # RUNTIME DESTINATION ${install_dir}   # Windows DLLs
)
# Install libs
install(TARGETS ripple_thd_qhpi_static_g0pic ripple_thd_qhpi_shared_g0pic
  ARCHIVE DESTINATION ${install_dir}/../G0/pic   # static libs (.a/.lib)
  LIBRARY DESTINATION ${install_dir}/../G0/pic   # shared libs (.so/.dylib)
  # RUNTIME DESTINATION ${install_dir}   # Windows DLLs
)

# __________________________ Common ripple thd files ___________________________

get_compiler_rt_install_dir(${COMPILER_RT_DEFAULT_TARGET_ARCH} install_dir)

message(NOTICE "RIPPLE THD INSTALL DIR is ${install_dir}")

install(
  FILES
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_block.h
  ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_conventions.h
  COMPONENT compiler-rt-headers
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION
  ${COMPILER_RT_INSTALL_INCLUDE_DIR}/ripple
)

# __________________________ Environment detection _____________________________

# QuRT

include(../ripple-hexagon.cmake)

set(HEXAGON_SDK_ROOT $ENV{HEXAGON_SDK_ROOT})
if(NOT "${HEXAGON_SDK_ROOT}" STREQUAL "")
  message(NOTICE "Using HEXAGON_SDK_ROOT: ${HEXAGON_SDK_ROOT}")
  if ("${HEXAGON_ARCH_VERSION}" STREQUAL "")
    set(HEXAGON_ARCH_VERSION $ENV{HEXAGON_ARCH_VERSION})
  endif()
  if(NOT "${HEXAGON_ARCH_VERSION}" STREQUAL "")
    message(NOTICE "Targeting HEXAGON_ARCH_VERSION: ${HEXAGON_ARCH_VERSION}")
    set_hexagon_version_info(
      "${HEXAGON_ARCH_VERSION}"
      HEXAGON_VERSION_FLAGS
      QURT_COMPUTE_PATH)

    set(_hexagon_qurt_dir
      "${HEXAGON_SDK_ROOT}/rtos/qurt/${QURT_COMPUTE_PATH}/include/qurt"
    )
    message(NOTICE "compute dir is ${_hexagon_qurt_dir}")
    if(EXISTS "${_hexagon_qurt_dir}" AND IS_DIRECTORY "${_hexagon_qurt_dir}")
      message(STATUS "Found QuRT include folder: ${_hexagon_qurt_dir}")
      set(HEXAGON_QURT_INCLUDE_DIR
        "${_hexagon_qurt_dir}/" CACHE PATH "QuRT include directory" FORCE
      )
    else()
      message(WARNING "No QuRT include folder found in the Hexagon SDK")
    endif()
  else()
    message(WARNING "Hexagon SDK root found, but HEXAGON_ARCH_VERSION undefined")
  endif()

  # Detect the tool set
  if ("${HEXAGON_TOOLS_ROOT}" STREQUAL "")
    set (HEXAGON_TOOLS_ROOT $ENV{HEXAGON_TOOLS_ROOT})
    if ("${HEXAGON_TOOLS_ROOT}" STREQUAL "")
      set (HEXAGON_TOOLS_ROOT $ENV{DEFAULT_HEXAGON_TOOLS_ROOT})
      if ("${HEXAGON_TOOLS_ROOT}" STREQUAL "")
        message(ERROR "Hexagon tools root folder not found. "
        "Did you setup the SDK environment variables (setup_sdk_env.source) ?"
        "You can also define $HEXAGON_TOOLS_ROOT.")
      endif()
    endif()
  endif()
  message(NOTICE "HEXAGON_TOOLS_ROOT=${HEXAGON_TOOLS_ROOT}")
endif()

set(HEXAGON_FLAGS
    -O2
    -Wall
    -m${HEXAGON_ARCH_VERSION}
    -Wno-gnu-include-next
    -fenable-ripple
    )


# __________________________ QuRT-based thd lib ________________________________


# Build and install QuRT-based runtime and headers

# If we have a QuRT include dir, we know that we also have an sdk and a toolset
if (NOT "${HEXAGON_QURT_INCLUDE_DIR}" STREQUAL "")
  message(NOTICE "Building QuRT-based Ripple thd lib")

  # Sources
  set(RIPPLE_THD_QURT_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/qurt/ripple_thd_qurt.c"
    "${CMAKE_CURRENT_LIST_DIR}/qurt/ripple_thd_qurt_pool.c"
    "${CMAKE_CURRENT_LIST_DIR}/ripple_thd_it_serv.c"
    "${CMAKE_CURRENT_LIST_DIR}/ripple_thd.c"
  )

  # Include dirs needed to compile the sources
  set(RIPPLE_THD_QURT_INCLUDE_DIRS
    "${HEXAGON_TOOLS_ROOT}/target/hexagon/include"
    "${HEXAGON_QURT_INCLUDE_DIR}"
    "${HEXAGON_QURT_INCLUDE_DIR}/../posix"
    "${CMAKE_CURRENT_LIST_DIR}/header"
  )

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
  target_compile_definitions(ripple_thd_qurt_static
  PRIVATE
    RIPPLE_THD_USE_QURT
  )
  target_compile_options(ripple_thd_qurt_static
  PRIVATE
    ${HEXAGON_FLAGS}
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
  target_compile_definitions(ripple_thd_qurt_static_g0pic
  PRIVATE
    RIPPLE_THD_USE_QURT
  )
  target_compile_options(ripple_thd_qurt_static_g0pic
  PRIVATE
    ${HEXAGON_FLAGS}
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
  target_compile_definitions(ripple_thd_qurt_shared
  PRIVATE
    RIPPLE_THD_USE_QURT
  )
  target_compile_options(ripple_thd_qurt_shared
  PRIVATE
    ${HEXAGON_FLAGS}
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
  target_compile_definitions(ripple_thd_qurt_shared_g0pic
  PRIVATE
    RIPPLE_THD_USE_QURT
  )
  target_compile_options(ripple_thd_qurt_shared_g0pic
  PRIVATE
    ${HEXAGON_FLAGS}
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
    ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_qurt.h
    COMPONENT compiler-rt-headers
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION
    ${COMPILER_RT_INSTALL_INCLUDE_DIR}/ripple
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


  # _ _ _ _ _ _ _ _ _ _ _ _ _  QHPI-based thd libs _ _ _ _ _ _ _ _ _ _ _ _ _ _ _

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
    "${HEXAGON_QURT_INCLUDE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/header"
  )

  # If QHPI headers exist, use them
  set(HNN_ROOT $ENV{HNN_ROOT})
  if(DEFINED HNN_ROOT AND EXISTS "${HNN_ROOT}" AND IS_DIRECTORY "${HNN_ROOT}")
    set(QHPI_INCLUDE "${HNN_ROOT}/hexagon/include/pub")
  endif()

  set(RIPPLE_THD_QHPI_EXPORT_HEADERS
    "${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_qhpi.h"
  )

  if(DEFINED QHPI_INCLUDE AND EXISTS "${QHPI_INCLUDE}" AND IS_DIRECTORY "${QHPI_INCLUDE}")
    list(APPEND RIPPLE_THD_QHPI_INCLUDE_DIRS "${QHPI_INCLUDE}")
  endif()

  # Keep targets that share OUTPUT_NAME in distinct build-tree directories.
  # They install to distinct destinations, but CMake/Ninja still needs unique
  # build-tree output paths.
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
    ${HEXAGON_FLAGS}
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
    ${HEXAGON_FLAGS}
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
  if(DEFINED QHPI_INCLUDE AND EXISTS "${QHPI_INCLUDE}" AND IS_DIRECTORY "${QHPI_INCLUDE}")
    target_compile_definitions(ripple_thd_qhpi_static PRIVATE __HAS_QHPI__)
    target_compile_definitions(ripple_thd_qhpi_shared PRIVATE __HAS_QHPI__)
  else()
    message(NOTICE "Building QHPI-based ripple_thd without QHPI headers")
  endif()

  target_compile_options(ripple_thd_qhpi_shared
  PRIVATE
    ${HEXAGON_FLAGS}
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
  if(DEFINED QHPI_INCLUDE AND EXISTS "${QHPI_INCLUDE}" AND IS_DIRECTORY "${QHPI_INCLUDE}")
    target_compile_definitions(ripple_thd_qhpi_static_g0pic PRIVATE __HAS_QHPI__)
    target_compile_definitions(ripple_thd_qhpi_shared_g0pic PRIVATE __HAS_QHPI__)
  else()
    message(NOTICE "Building QHPI-based ripple_thd without QHPI headers")
  endif()

  target_compile_options(ripple_thd_qhpi_shared_g0pic
  PRIVATE
    ${HEXAGON_FLAGS}
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
    ${CMAKE_CURRENT_LIST_DIR}/header/ripple/ripple_thd_qhpi.h
    ${RIPPLE_THD_QHPI_EXPORT_HEADERS}
    COMPONENT compiler-rt-headers
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION
    ${COMPILER_RT_INSTALL_INCLUDE_DIR}/ripple
  )

  # Install libs
  install(TARGETS ripple_thd_qhpi_static ripple_thd_qhpi_shared
    ARCHIVE DESTINATION ${install_dir}   # static libs (.a/.lib)
    LIBRARY DESTINATION ${install_dir}   # shared libs (.so/.dylib)
    # RUNTIME DESTINATION ${install_dir}   # Windows DLLs
  )
  # Install G0 + pic libs
  install(TARGETS ripple_thd_qhpi_static_g0pic ripple_thd_qhpi_shared_g0pic
    ARCHIVE DESTINATION ${install_dir}/../G0/pic   # static libs (.a/.lib)
    LIBRARY DESTINATION ${install_dir}/../G0/pic   # shared libs (.so/.dylib)
    # RUNTIME DESTINATION ${install_dir}   # Windows DLLs
  )

else() # QuRT include dir doesn't exist
  message(NOTICE "Not building QuRT-based Ripple thd lib")
endif()

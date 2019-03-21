# Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

INCLUDE (CheckCSourceCompiles)
INCLUDE (CheckCXXSourceCompiles)
INCLUDE (CheckStructHasMember)
INCLUDE (CheckLibraryExists)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckCCompilerFlag)
INCLUDE (CheckCSourceRuns)
INCLUDE (CheckCXXSourceRuns)
INCLUDE (CheckSymbolExists)


# WITH_PIC options.Not of much use, PIC is taken care of on platforms
# where it makes sense anyway.
IF(UNIX)
  IF(APPLE)
    # OSX  executable are always PIC
    SET(WITH_PIC ON)
  ELSE()
    OPTION(WITH_PIC "Generate PIC objects" OFF)
    IF(WITH_PIC)
      SET(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} ${CMAKE_SHARED_LIBRARY_C_FLAGS}")
      SET(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${CMAKE_SHARED_LIBRARY_CXX_FLAGS}")
    ENDIF()
  ENDIF()
ENDIF()


IF(CMAKE_SYSTEM_NAME MATCHES "SunOS")
  ## Add C flags (e.g. -m64) to CMAKE_SHARED_LIBRARY_C_FLAGS
  ## The client library contains C++ code, so add dependency on libstdc++
  ## See cmake --help-policy CMP0018
  SET(CMAKE_SHARED_LIBRARY_C_FLAGS
    "${CMAKE_SHARED_LIBRARY_C_FLAGS} ${CMAKE_C_FLAGS} -lstdc++")
ENDIF()


# System type affects version_compile_os variable
IF(NOT SYSTEM_TYPE)
  IF(PLATFORM)
    SET(SYSTEM_TYPE ${PLATFORM})
  ELSE()
    SET(SYSTEM_TYPE ${CMAKE_SYSTEM_NAME})
  ENDIF()
ENDIF()

# Check to see if we are using LLVM's libc++ rather than e.g. libstd++
# Can then check HAVE_LLBM_LIBCPP later without including e.g. ciso646.
CHECK_CXX_SOURCE_RUNS("
#include <ciso646>
int main()
{
#ifdef _LIBCPP_VERSION
  return 0;
#else
  return 1;
#endif
}" HAVE_LLVM_LIBCPP)

MACRO(DIRNAME IN OUT)
  GET_FILENAME_COMPONENT(${OUT} ${IN} PATH)
ENDMACRO()

MACRO(EXTEND_CXX_LINK_FLAGS LIBRARY_PATH)
  # Using the $ORIGIN token with the -R option to locate the libraries
  # on a path relative to the executable:
  # We need an extra backslash to pass $ORIGIN to the mysql_config script...
  SET(QUOTED_CMAKE_CXX_LINK_FLAGS
    "${CMAKE_CXX_LINK_FLAGS} -R'\\$ORIGIN/../lib' -R${LIBRARY_PATH}")
  SET(CMAKE_CXX_LINK_FLAGS
    "${CMAKE_CXX_LINK_FLAGS} -R'\$ORIGIN/../lib' -R${LIBRARY_PATH}")
  MESSAGE(STATUS "CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS}")
ENDMACRO()

MACRO(EXTEND_C_LINK_FLAGS LIBRARY_PATH)
  SET(QUOTED_CMAKE_C_LINK_FLAGS
    "${CMAKE_C_LINK_FLAGS} -R'\\$ORIGIN/../lib' -R${LIBRARY_PATH}")
  SET(CMAKE_C_LINK_FLAGS
    "${CMAKE_C_LINK_FLAGS} -R'\$ORIGIN/../lib' -R${LIBRARY_PATH}")
  MESSAGE(STATUS "CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS}")
  SET(CMAKE_SHARED_LIBRARY_C_FLAGS
    "${CMAKE_SHARED_LIBRARY_C_FLAGS} -R'\$ORIGIN/../lib' -R${LIBRARY_PATH}")
ENDMACRO()

IF(CMAKE_COMPILER_IS_GNUCXX)
  IF (CMAKE_EXE_LINKER_FLAGS MATCHES " -static "
     OR CMAKE_EXE_LINKER_FLAGS MATCHES " -static$")
     SET(HAVE_DLOPEN FALSE CACHE "Disable dlopen due to -static flag" FORCE)
     SET(WITHOUT_DYNAMIC_PLUGINS TRUE)
  ENDIF()
ENDIF()

IF(WITHOUT_DYNAMIC_PLUGINS)
  MESSAGE("Dynamic plugins are disabled.")
ENDIF(WITHOUT_DYNAMIC_PLUGINS)

# Large files, common flag
SET(_LARGEFILE_SOURCE  1)

# Same for structs, setting HAVE_STRUCT_<name> instead
FUNCTION(MY_CHECK_STRUCT_SIZE type defbase)
  CHECK_TYPE_SIZE("struct ${type}" SIZEOF_${defbase})
  IF(SIZEOF_${defbase})
    SET(HAVE_STRUCT_${defbase} 1 PARENT_SCOPE)
  ENDIF()
ENDFUNCTION()

# Searches function in libraries
# if function is found, sets output parameter result to the name of the library
# if function is found in libc, result will be empty
FUNCTION(MY_SEARCH_LIBS func libs result)
  IF(${${result}})
    # Library is already found or was predefined
    RETURN()
  ENDIF()
  CHECK_FUNCTION_EXISTS(${func} HAVE_${func}_IN_LIBC)
  IF(HAVE_${func}_IN_LIBC)
    SET(${result} "" PARENT_SCOPE)
    RETURN()
  ENDIF()
  FOREACH(lib  ${libs})
    CHECK_LIBRARY_EXISTS(${lib} ${func} "" HAVE_${func}_IN_${lib})
    IF(HAVE_${func}_IN_${lib})
      SET(${result} ${lib} PARENT_SCOPE)
      SET(HAVE_${result} 1 PARENT_SCOPE)
      RETURN()
    ENDIF()
  ENDFOREACH()
ENDFUNCTION()

# Find out which libraries to use.

# Figure out threading library
# Defines CMAKE_USE_PTHREADS_INIT and CMAKE_THREAD_LIBS_INIT.
FIND_PACKAGE (Threads)

IF(UNIX)
  MY_SEARCH_LIBS(floor m LIBM)
  IF(NOT LIBM)
    MY_SEARCH_LIBS(__infinity m LIBM)
  ENDIF()
  MY_SEARCH_LIBS(gethostbyname_r  "nsl_r;nsl" LIBNSL)
  MY_SEARCH_LIBS(bind "bind;socket" LIBBIND)
  MY_SEARCH_LIBS(crypt crypt LIBCRYPT)
  MY_SEARCH_LIBS(setsockopt socket LIBSOCKET)
  MY_SEARCH_LIBS(dlopen dl LIBDL)
  MY_SEARCH_LIBS(sched_yield rt LIBRT)
  IF(NOT LIBRT)
    MY_SEARCH_LIBS(clock_gettime rt LIBRT)
  ENDIF()
  MY_SEARCH_LIBS(timer_create rt LIBRT)

  SET(CMAKE_REQUIRED_LIBRARIES
    ${LIBM} ${LIBNSL} ${LIBBIND} ${LIBCRYPT} ${LIBSOCKET} ${LIBDL} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT})
  # Need explicit pthread for gcc -fsanitize=address
  IF(CMAKE_C_FLAGS MATCHES "-fsanitize=")
    SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} pthread)
  ENDIF()

  LIST(LENGTH CMAKE_REQUIRED_LIBRARIES required_libs_length)
  IF(${required_libs_length} GREATER 0)
    LIST(REMOVE_DUPLICATES CMAKE_REQUIRED_LIBRARIES)
  ENDIF()
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS_INIT})

  OPTION(WITH_LIBWRAP "Compile with tcp wrappers support" OFF)
  IF(WITH_LIBWRAP)
    SET(SAVE_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
    SET(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} wrap)
    CHECK_C_SOURCE_COMPILES(
    "
    #include <tcpd.h>
    int allow_severity = 0;
    int deny_severity  = 0;
    int main()
    {
      hosts_access(0);
    }"
    HAVE_LIBWRAP)
    SET(CMAKE_REQUIRED_LIBRARIES ${SAVE_CMAKE_REQUIRED_LIBRARIES})
    IF(HAVE_LIBWRAP)
      SET(LIBWRAP "wrap")
    ELSE()
      MESSAGE(FATAL_ERROR
      "WITH_LIBWRAP is defined, but can not find a working libwrap. "
      "Make sure both the header files (tcpd.h) "
      "and the library (libwrap) are installed.")
    ENDIF()
  ENDIF()
ENDIF()


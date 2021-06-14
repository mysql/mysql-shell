# Copyright (c) 2015, 2021, Oracle and/or its affiliates.
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

include(CheckCXXCompilerFlag)

# Check for C++ 17 support
set(CXX_STD "17")

macro(APPEND_FLAG _string_var _addition)
  set(${_string_var} "${${_string_var}} ${_addition}")
endmacro()

macro(APPEND_MISSING_FLAG _string_var _addition)
  string(FIND "${_string_var}" "${_addition}" _pos)
  if(_pos LESS 0)
    set(${_string_var} "${${_string_var}} ${_addition}")
  endif()
endmacro()

function(CHECK_CXX_STD)
  IF(MSVC)
    SET(CXX_STD_FLAG "/std:c++${CXX_STD}")
  ELSE()
    SET(CXX_STD_FLAG "-std=c++${CXX_STD}")
  ENDIF()

  check_cxx_compiler_flag("${CXX_STD_FLAG}" supports_std)

  if(supports_std)
    IF(MSVC)
      FOREACH(flag
              CMAKE_CXX_FLAGS_MINSIZEREL
              CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO
              CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG_INIT)
        SET("${flag}" "${${flag}} ${CXX_STD_FLAG}" PARENT_SCOPE)
      ENDFOREACH()
    ENDIF()
  else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} does not support C++${CXX_STD} standard")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STD_FLAG}" PARENT_SCOPE)
endfunction()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  CHECK_CXX_STD()
  #set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wpedantic -Wunused -Wshadow -Wdouble-promotion -Wformat-security -Wformat-y2k")

  if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wlogical-op -Wno-error=type-limits")
  endif()

  if(ENABLE_GCOV)
    message(STATUS "Enabling code coverage using Gcov")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  endif()

elseif(MSVC)
  # Overview of MSVC versions: http://www.cmake.org/cmake/help/v3.3/variable/MSVC_VERSION.html
  if(MSVC_VERSION LESS 1900)
    message(FATAL_ERROR "Need at least Visual Studio 2015")
  endif()

  message(STATUS "Using Visual Studio version: ${MSVC_VERSION} (${MSVC_TOOLSET_VERSION})")

  CHECK_CXX_STD()

  # /TP is needed so .cc files are recognized as C++ source files by MSVC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /TP")

  if(ENABLE_GCOV)
    message(FATAL_ERROR "Code coverage not supported with MSVC")
  endif()

elseif(CMAKE_C_COMPILER_ID MATCHES "SunPro" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.15")

  # 5.15 is Oracle Developer Studio 12.6

  foreach(_flag CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
    APPEND_MISSING_FLAG(${_flag} "-xbuiltin=%all")
    APPEND_MISSING_FLAG(${_flag} "-xlibmil")
    APPEND_MISSING_FLAG(${_flag} "-xatomic=studio")
  endforeach()

  APPEND_MISSING_FLAG(CMAKE_CXX_FLAGS "-std=c++${CXX_STD}")

  foreach(_flag CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_C_LINK_FLAGS CMAKE_CXX_LINK_FLAGS)
    if(${_flag} MATCHES "-m32")
      message(FATAL_ERROR "32-bit is not supported on Solaris")
    endif()
    APPEND_MISSING_FLAG(${_flag} "-m64")
  endforeach()

  add_definitions(-D_POSIX_PTHREAD_SEMANTICS)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "sparc")
    # Work around a bug in PROTOBUF 2.6.1
    add_definitions(-DSOLARIS_64BIT_ENABLED)
  endif()

  # Reduce size of debug binaries, by omitting function declarations.
  # Note that we cannot set "-xdebuginfo=no%decl" during feature tests.
  # Linking errors for merge_large_tests-t with Studio 12.6
  # -g0 is the same as -g, except that inlining is enabled.
  # When building -DWITH_NDBCLUSTER=1 even more of the merge_xxx_tests
  # fail to link, so we keep -g0 for Studio 12.6
  # FIXME most above might not apply for Shell
  APPEND_FLAG(CMAKE_C_FLAGS_DEBUG   "-g0")
  APPEND_FLAG(CMAKE_CXX_FLAGS_DEBUG "-g0")
  APPEND_MISSING_FLAG(CMAKE_C_FLAGS_DEBUG            "-xdebuginfo=no%decl")
  APPEND_MISSING_FLAG(CMAKE_CXX_FLAGS_DEBUG          "-xdebuginfo=no%decl")
  APPEND_MISSING_FLAG(CMAKE_C_FLAGS_RELWITHDEBINFO   "-xdebuginfo=no%decl")
  APPEND_MISSING_FLAG(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-xdebuginfo=no%decl")

  # Bugs in SunPro, compile/link error unless we add some debug info.
  # Errors seem to be related to TLS functions.
  APPEND_FLAG(CMAKE_CXX_FLAGS_MINSIZEREL "-g0")
  APPEND_FLAG(CMAKE_CXX_FLAGS_RELEASE    "-g0")
  APPEND_MISSING_FLAG(CMAKE_CXX_FLAGS_MINSIZEREL
                      "-xdebuginfo=no%line,no%param,no%decl,no%variable,no%tagtype")
  APPEND_MISSING_FLAG(CMAKE_CXX_FLAGS_RELEASE
                      "-xdebuginfo=no%line,no%param,no%decl,no%variable,no%tagtype")

else()
  message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} is not supported")
endif()

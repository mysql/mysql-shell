# Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

# Check for C++ 11 support
include(CheckCXXCompilerFlag)

function(CHECK_CXX11)
  check_cxx_compiler_flag("-std=c++11" support_11)

  if(support_11)
    set(CXX11_FLAG "-std=c++11" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} does not support C++11 standard")
  endif()
  set(CMAKE_CXX_FLAGS ${CXX11_FLAG} PARENT_SCOPE)
endfunction()

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  check_cxx11()
  #set(${CMAKE_CXX_FLAGS} "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -Wconversion -Wpedantic -Wshadow")

  # Flags to use in old parts of the code, where we have too many warnings
  # as result of the typo above. We incrementally add warnings until everything is on
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAG} -Wall -Wno-unused-parameter -Wno-unused-result")
  # Flags to use in new parts of the code, where we're trying to be strict from the beginning
  set(CXX_FLAGS_FULL_WARNINGS "${CMAKE_CXX_FLAGS} -Werror -Wextra -Wno-shadow")

  if(ENABLE_GCOV)
    message(STATUS "Enabling code coverage using Gcov")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
  endif()

elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # Overview of MSVC versions: http://www.cmake.org/cmake/help/v3.3/variable/MSVC_VERSION.html
  if("${MSVC_VERSION}" VERSION_LESS 1800)
    message(FATAL_ERROR "Need at least ${CMAKE_CXX_COMPILER} 12.0")
  endif()
  # /TP is needed so .cc files are recognoized as C++ source files by MSVC
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /TP")

  if(ENABLE_GCOV)
    message(FATAL_ERROR "Code coverage not supported with MSVC")
  endif()
else()
  message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} is not supported")
endif()

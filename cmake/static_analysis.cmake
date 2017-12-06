# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

IF(USE_CLANG_TIDY)
  FIND_PROGRAM(CLANG_TIDY clang-tidy)
ENDIF()

# Register source files for static analysis
MACRO(ADD_STAN_TARGET TARGET)
  SET(STAN_TARGETS "${TARGET};${STAN_TARGETS}" CACHE INTERNAL "Targets for static analysis")

  IF(USE_CLANG_TIDY)
    GET_TARGET_PROPERTY(INCLUDE_DIRS ${TARGET} INCLUDE_DIRECTORIES)
    IF(APPLE)
      SET(INCLUDES "-isysroot;${CMAKE_OSX_SYSROOT}")
    ELSE()
      SET(INCLUDES "")
    ENDIF()
    FOREACH(DIR ${INCLUDE_DIRS})
      LIST(APPEND INCLUDES -I${DIR})
    ENDFOREACH()
    GET_TARGET_PROPERTY(DEFINES ${TARGET} COMPILE_DEFINITIONS)
    GET_TARGET_PROPERTY(OPTIONS ${TARGET} COMPILE_OPTIONS)
    ADD_CUSTOM_TARGET(tidy-${TARGET}
      ${CLANG_TIDY} ${CLANG_TIDY_FLAGS} ${ARGN} -config='' -- -x=c++ -std=c++11 -fexceptions ${INCLUDES} ${DEFINES} ${OPTIONS}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  ENDIF()
ENDMACRO()


SET(STAN_TARGETS "" CACHE INTERNAL "Targets for static analysis")

MACRO(CHECK_STAN_OPTIONS)
  # clang-tidy support. See http://clang.llvm.org/extra/clang-tidy/
  # (install clang to get it)
  #
  # Run cmake with -DUSE_CLANG_TIDY=1
  IF(USE_CLANG_TIDY)
    MESSAGE(STATUS "Enabling clang-tidy support with: ${CLANG_TIDY} CLANG_TIDY_FLAGS=${CLANG_TIDY_FLAGS}")
    MESSAGE(STATUS "Available targets:")
    FOREACH(TARGET ${STAN_TARGETS})
        MESSAGE(STATUS "    tidy-${TARGET}")
    ENDFOREACH()
  ENDIF()
ENDMACRO()

IF(USE_ASAN)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
ENDIF()

IF(SANITIZE_ALL)
  IF(APPLE)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=memory -fstack-protector-strong")
  ELSE()
    MESSAGE(ERROR "SANITIZE_ALL not supported in this platform")
  ENDIF()
ENDIF()

IF(FORCE_CLANG_COLOR)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -fcolor-diagnostics")
ENDIF()

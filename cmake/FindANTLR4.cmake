# Copyright (c) 2016, 2022, Oracle and/or its affiliates.
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
# - Find antlr
# Find the native ANTLR4 includes and library
#
#  ANTLR4_INCLUDE_DIRS - where to find sqlite/connection.hpp, etc.
#  ANTLR4_LIBRARIES    - List of libraries when using pcre.
#  ANTLR4_FOUND        - True if pcre found.


IF (ANTLR4_INCLUDE_DIRS)
  # Already in cache, be silent
  SET(ANTLR4_FIND_QUIETLY TRUE)
ENDIF (ANTLR4_INCLUDE_DIRS)

FIND_PATH(ANTLR4_INCLUDE_DIR antlr4-runtime/antlr4-runtime.h HINTS ${ANTLR4_INCLUDE_DIRS})

SET(ANTLR4_NAMES antlr4-runtime)

if(ANTLR4_LIBRARIES)
  # Converto to a list of library argments
  string(REPLACE " " ";" ANTLR4_LIB_ARGS ${ANTLR4_LIBRARIES})
  # Parse the list in order to find the library path
  foreach(ANTLR4_LIB_ARG ${ANTLR4_LIB_ARGS})
    string(REPLACE "-L" "" ANTLR4_SUPPLIED_LIB_DIR ${ANTLR4_LIB_ARG})
  endforeach(ANTLR4_LIB_ARG)
  find_library(ANTLR4_LIBRARY NAMES ${ANTLR4_NAMES} HINTS ${ANTLR4_SUPPLIED_LIB_DIR})

  unset(ANTLR4_LIB_ARG)
  unset(ANTLR4_LIB_ARGS)
else()
  find_library(ANTLR4_LIBRARY NAMES ${ANTLR4_NAMES})
endif()

# handle the QUIETLY and REQUIRED arguments and set ANTLR4_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ANTLR4 DEFAULT_MSG ANTLR4_LIBRARY ANTLR4_INCLUDE_DIR)

IF(ANTLR4_FOUND)
  get_filename_component(ANTLR4_LIBRARY_REAL ${ANTLR4_LIBRARY} REALPATH)
  get_filename_component(ANTLR4_LIB_FILENAME ${ANTLR4_LIBRARY_REAL} NAME)
  get_filename_component(ANTLR4_LIB_DIRECTORY ${ANTLR4_LIBRARY} PATH)
  get_filename_component(ANTLR4_LIB_BASE_DIRECTORY ${ANTLR4_LIB_DIRECTORY} PATH)
  set(ANTLR4_INCLUDE_DIR ${ANTLR4_INCLUDE_DIR}/antlr4-runtime)
  IF(WIN32)
    SET( ANTLR4_LIBRARIES "${ANTLR4_LIBRARY}" )
  ELSE()
    SET( ANTLR4_LIBRARIES "-L${ANTLR4_LIB_DIRECTORY} -l${ANTLR4_NAMES}" )
  ENDIF()
  SET( ANTLR4_INCLUDE_DIRS ${ANTLR4_INCLUDE_DIR} )
  # get version, use the version file first (4.10+)
  if(NOT DEFINED ANTLR4_VERSION)
    if(EXISTS "${ANTLR4_INCLUDE_DIRS}/Version.h")
      file(READ "${ANTLR4_INCLUDE_DIRS}/Version.h" ANTLR4_VERSION_FILE)
      string(REGEX MATCH "ANTLRCPP_VERSION_MAJOR ([0-9]+)" ANTLR4_VERSION_LINE "${ANTLR4_VERSION_FILE}")
      set(ANTLR4_VERSION_MAJOR "${CMAKE_MATCH_1}")
      string(REGEX MATCH "ANTLRCPP_VERSION_MINOR ([0-9]+)" ANTLR4_VERSION_LINE "${ANTLR4_VERSION_FILE}")
      set(ANTLR4_VERSION_MINOR "${CMAKE_MATCH_1}")
      string(REGEX MATCH "ANTLRCPP_VERSION_PATCH ([0-9]+)" ANTLR4_VERSION_LINE "${ANTLR4_VERSION_FILE}")
      set(ANTLR4_VERSION_PATCH "${CMAKE_MATCH_1}")
      set(ANTLR4_VERSION "${ANTLR4_VERSION_MAJOR}.${ANTLR4_VERSION_MINOR}.${ANTLR4_VERSION_PATCH}" CACHE INTERNAL "")
      set(ANTLR4_VERSION_SHORT "${ANTLR4_VERSION_MAJOR}.${ANTLR4_VERSION_MINOR}" CACHE INTERNAL "")
    endif()
  endif()
  # get version, compile and run a test binary (doesn't work with 4.10 due to a macro bug)
  if(NOT DEFINED ANTLR4_VERSION)
    SET(GET_ANTLR4_VERSION [[
#include <iostream>
#include <antlr4-runtime.h>
int main() {
  std::cout << antlr4::RuntimeMetaData::VERSION << std::endl;
  return 0;
}
]])
    SET(GET_ANTLR4_VERSION_FILE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.cc")
    file(WRITE "${GET_ANTLR4_VERSION_FILE}" "${GET_ANTLR4_VERSION}")
    try_run(GET_ANTLR4_VERSION_EXITCODE GET_ANTLR4_VERSION_COMPILED
      ${CMAKE_BINARY_DIR}
      ${GET_ANTLR4_VERSION_FILE}
      CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${ANTLR4_INCLUDE_DIR} -DLINK_DIRECTORIES=${ANTLR4_LIB_DIRECTORY} -DLINK_LIBRARIES=${ANTLR4_NAMES}
      COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
      RUN_OUTPUT_VARIABLE RUN_OUTPUT
    )
    if(NOT ${GET_ANTLR4_VERSION_COMPILED})
      set(${GET_ANTLR4_VERSION_EXITCODE} 1)
    endif()
    if(${GET_ANTLR4_VERSION_COMPILED} AND ${GET_ANTLR4_VERSION_EXITCODE} EQUAL 0)
      string(STRIP "${RUN_OUTPUT}" RUN_OUTPUT)
      set(ANTLR4_VERSION "${RUN_OUTPUT}")
      string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" ANTLR4_VERSION_SHORT "${ANTLR4_VERSION}")
      set(ANTLR4_VERSION_SHORT "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")
      set(ANTLR4_VERSION "${ANTLR4_VERSION}" CACHE INTERNAL "")
      set(ANTLR4_VERSION_SHORT "${ANTLR4_VERSION_SHORT}" CACHE INTERNAL "")
    else()
      message(FATAL_ERROR "Could not get ANTLR4 version, compile output:\n${COMPILE_OUTPUT}\nrun output:\n${RUN_OUTPUT}")
    endif()
  endif()
ELSE(ANTLR4_FOUND)
  SET( ANTLR4_LIBRARIES )
  SET( ANTLR4_INCLUDE_DIRS )
  SET( ANTLR4_VERSION )
  SET( ANTLR4_VERSION_SHORT )
ENDIF(ANTLR4_FOUND)

message("ANTLR4 INCLUDE DIR: ${ANTLR4_INCLUDE_DIRS}")
message("ANTLR4 LIB DIR: ${ANTLR4_LIBRARIES}")
message("ANTLR4 VERSION: ${ANTLR4_VERSION}")
message("ANTLR4 SHORT VERSION: ${ANTLR4_VERSION_SHORT}")

MARK_AS_ADVANCED( ANTLR4_LIBRARIES ANTLR4_INCLUDE_DIRS )

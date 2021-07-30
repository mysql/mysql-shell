# Copyright (c) 2020, 2021, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

# cmake -DWITH_ZSTD=bundled|system
# bundled is the default

macro (FIND_SYSTEM_ZSTD)
  find_path(PATH_TO_ZSTD NAMES zstd.h)
  find_library(ZSTD_SYSTEM_LIBRARY NAMES zstd)
  if(PATH_TO_ZSTD AND ZSTD_SYSTEM_LIBRARY)
    set(SYSTEM_ZSTD_FOUND 1)
    include_directories(SYSTEM ${PATH_TO_ZSTD})
    set(ZSTD_LIBRARY ${ZSTD_SYSTEM_LIBRARY})
    message(STATUS "PATH_TO_ZSTD ${PATH_TO_ZSTD}")
    message(STATUS "ZSTD_LIBRARY ${ZSTD_LIBRARY}")
  endif()
endmacro()

macro(MYSQL_USE_BUNDLED_ZSTD)
  if(MYSQL_SOURCE_DIR AND MYSQL_BUILD_DIR)
    set(WITH_ZSTD "bundled" CACHE STRING "By default use bundled zstd library")
    set(BUILD_BUNDLED_ZSTD 1)
    file(GLOB_RECURSE ZSTD_INCLUDE_FILE ${MYSQL_SOURCE_DIR}/extra/zstd/*/lib/zstd.h)

    if(NOT ZSTD_INCLUDE_FILE)
      message(FATAL_ERROR "Could not find \"zstd.h\"")
    endif()

    get_filename_component(ZSTD_INCLUDE_DIR ${ZSTD_INCLUDE_FILE} DIRECTORY)
    include_directories(BEFORE SYSTEM ${ZSTD_INCLUDE_DIR})

    if(WIN32)
      find_file(ZSTD_LIBRARY NAMES zstd.lib PATHS "${MYSQL_BUILD_DIR}/archive_output_directory/"
        PATH_SUFFIXES ${CMAKE_BUILD_TYPE} RelWithDebInfo Release Debug)
    else()
      find_file(ZSTD_LIBRARY NAMES libzstd.a PATHS "${MYSQL_BUILD_DIR}/archive_output_directory/")
    endif()
  endif()
endmacro()

if(NOT WITH_ZSTD)
  set(WITH_ZSTD "bundled" CACHE STRING "By default use bundled zstd library")
endif()

macro(MYSQL_CHECK_ZSTD)
  if(WITH_ZSTD STREQUAL "bundled")
    MYSQL_USE_BUNDLED_ZSTD()
  elseif(WITH_ZSTD STREQUAL "system")
    FIND_SYSTEM_ZSTD()
    if(NOT SYSTEM_ZSTD_FOUND)
      message(FATAL_ERROR "Cannot find system zstd libraries.")
    endif()
  else()
    message(FATAL_ERROR "WITH_ZSTD must be bundled or system")
  endif()
endmacro()

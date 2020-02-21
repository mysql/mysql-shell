# Copyright (c) 2020, Oracle and/or its affiliates. All rights reserved.
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

# cmake -DWITH_ZLIB=bundled|system
# bundled is the default

macro (FIND_SYSTEM_ZLIB)
  find_path(PATH_TO_ZLIB NAMES zlib.h zconf.h)
  find_library(ZLIB_SYSTEM_LIBRARY NAMES z)
  if(PATH_TO_ZLIB AND ZLIB_SYSTEM_LIBRARY)
    set(SYSTEM_ZLIB_FOUND 1)
    include_directories(SYSTEM ${PATH_TO_ZLIB})
    set(ZLIB_LIBRARY ${ZLIB_SYSTEM_LIBRARY})
    message(STATUS "PATH_TO_ZLIB ${PATH_TO_ZLIB}")
    message(STATUS "ZLIB_LIBRARY ${ZLIB_LIBRARY}")
  endif()
endmacro()

macro(MYSQL_USE_BUNDLED_ZLIB)
  if(MYSQL_SOURCE_DIR AND MYSQL_BUILD_DIR)
    set(WITH_ZLIB "bundled" CACHE STRING "By default use bundled zlib library")
    set(BUILD_BUNDLED_ZLIB 1)
    include_directories(BEFORE SYSTEM ${MYSQL_SOURCE_DIR}/extra/zlib ${MYSQL_BUILD_DIR}/extra/zlib)

    if(WIN32)
      find_file(ZLIB_LIBRARY NAMES zlib.lib PATHS "${MYSQL_BUILD_DIR}/archive_output_directory/"
        PATH_SUFFIXES ${CMAKE_BUILD_TYPE} RelWithDebInfo Release Debug)
    else()
      find_file(ZLIB_LIBRARY NAMES libz.a PATHS "${MYSQL_BUILD_DIR}/archive_output_directory/")
    endif()
  endif()
endmacro()

if(NOT WITH_ZLIB)
  set(WITH_ZLIB "bundled" CACHE STRING "By default use bundled zlib library")
endif()

macro(MYSQL_CHECK_ZLIB)
  if(WITH_ZLIB STREQUAL "bundled")
    MYSQL_USE_BUNDLED_ZLIB()
  elseif(WITH_ZLIB STREQUAL "system")
    FIND_SYSTEM_ZLIB()
    if(NOT SYSTEM_ZLIB_FOUND)
      message(FATAL_ERROR "Cannot find system zlib libraries.")
    endif()
  else()
    message(FATAL_ERROR "WITH_ZLIB must be bundled or system")
  endif()
endmacro()

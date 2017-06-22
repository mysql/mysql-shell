# Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

# This is a small CMake project to copy this directory to the top of
# the source, and at the same time do some modifications to these
# files

cmake_minimum_required(VERSION 2.8.5)
project(mysh_deb_init NONE)

# ----------------------------------------------------------------------
# Set some variables to replace
# Use the version variables from "version.cmake"
# ----------------------------------------------------------------------

set(ROOT_PROJECT_DIR "${CMAKE_SOURCE_DIR}/../..")
include(../../version.cmake)

set(YEAR      2017)  # FIXME automate somehow
set(PRODUCT   "MySQL Shell ${MYSH_BASE_VERSION}")

if(NOT DEBIAN_REVISION)
  set(DEBIAN_REVISION 1)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/../../LICENSE.mysql")
  set(PRODUCT_SUFFIX "-commercial")
  set(LIC_FILE "LICENSE.mysql")
  set(VERSION "${MYSH_VERSION}+commercial-${DEBIAN_REVISION}")
else()
  set(PRODUCT_SUFFIX "")
  set(LIC_FILE "COPYING.txt")
  set(VERSION "${MYSH_VERSION}-${DEBIAN_REVISION}")
endif()

# ----------------------------------------------------------------------
# Find out if Debian/Ubuntu, the codename, distribution and version
# ----------------------------------------------------------------------

execute_process(
  COMMAND lsb_release --short --id
  OUTPUT_VARIABLE lsb_id
  RESULT_VARIABLE lsb_result
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT lsb_result EQUAL 0)
  message(FATAL_ERROR "Can't run lsb_release")
endif()

string(TOLOWER "${lsb_id}" lsb_id)

if(NOT lsb_id STREQUAL "debian" AND NOT lsb_id STREQUAL "ubuntu")
  message(FATAL_ERROR "We can only handle Debian or Ubuntu Deb packaging")
endif()

execute_process(
  COMMAND lsb_release --short --release
  OUTPUT_VARIABLE lsb_release
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND lsb_release --short --codename
  OUTPUT_VARIABLE CODENAME
  OUTPUT_STRIP_TRAILING_WHITESPACE
)


if(lsb_id STREQUAL "debian")
  # For Debian we want just the major release number
  string(REGEX REPLACE "\\..*" "" lsb_release "${lsb_release}")
endif()

set(ID_RELEASE "${lsb_id}${lsb_release}")

message(STATUS "OS distribution : ${ID_RELEASE}")
message(STATUS "OS code name    : ${CODENAME}")

# ----------------------------------------------------------------------
# Create the argument list to the "cmake" call and possibly extra deps
# if not passed as argument
# ----------------------------------------------------------------------

set(EXTRA_CMAKE_OPTS)
set(DEB_BUILD_DEPS)

if(WITH_SSL)
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_SSL=${WITH_SSL}")
else()
  list(APPEND DEB_BUILD_DEPS "openssl")
endif()

if(MYSQLCLIENT_STATIC_LINKING OR ENV{MYSQLCLIENT_STATIC_LINKING})
  list(APPEND EXTRA_CMAKE_OPTS "-DMYSQLCLIENT_STATIC_LINKING=ON")
endif()

if(NOT MYSQL_DIR AND NOT MYSQL_INCLUDE_DIR AND NOT MYSQL_LIB_DIR AND
   "$ENV{MYSQL_DIR}" AND NOT "$ENV{MYSQL_INCLUDE_DIR}" AND NOT "$ENV{MYSQL_LIB_DIR}")
  list(APPEND DEB_BUILD_DEPS "libmysqlclient-dev")
endif()

if(BOOST_ROOT)
  list(APPEND EXTRA_CMAKE_OPTS "-DBOOST_ROOT=${BOOST_ROOT}")
  list(APPEND EXTRA_CMAKE_OPTS "-DBoost_NO_SYSTEM_PATHS:BOOL=TRUE")
else()
  list(APPEND DEB_BUILD_DEPS "libboost-all-dev")
endif()

if(WITH_PROTOBUF)
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_PROTOBUF=${WITH_PROTOBUF}")
else()
  list(APPEND DEB_BUILD_DEPS "libprotobuf-dev")
  list(APPEND DEB_BUILD_DEPS "protobuf-compiler")
endif()

if(V8_INCLUDE_DIR)
  list(APPEND EXTRA_CMAKE_OPTS "-DV8_INCLUDE_DIR=${V8_INCLUDE_DIR}")
endif()
if(V8_LIB_DIR)
  list(APPEND EXTRA_CMAKE_OPTS "-DV8_LIB_DIR=${V8_LIB_DIR}")
endif()

if(HAVE_PYTHON)
  list(APPEND EXTRA_CMAKE_OPTS "-DHAVE_PYTHON=1")
  list(APPEND DEB_BUILD_DEPS "python-dev")
endif()

if(WITH_CONNECTOR_PYTHON)
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_CONNECTOR_PYTHON=${WITH_CONNECTOR_PYTHON}")
endif()

if(WITH_GTEST)
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_TESTS=ON")
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_GTEST=${WITH_GTEST}")
endif()

string(REPLACE ";" " "  EXTRA_CMAKE_OPTS "${EXTRA_CMAKE_OPTS}")
string(REPLACE ";" ", " DEB_BUILD_DEPS   "${DEB_BUILD_DEPS}")

# ----------------------------------------------------------------------
# Copy this directory and process the .in files
# ----------------------------------------------------------------------

set(DEST_DIR ${CMAKE_SOURCE_DIR}/../../debian)

file(
  COPY ${CMAKE_SOURCE_DIR}/
  DESTINATION ${DEST_DIR}
  PATTERN "*.in"   EXCLUDE
  PATTERN "CMake*" EXCLUDE
)

set(in_files
  changelog
  control
  copyright
  rules
  mysql-shellPRODUCTSUFFIX.install
)

foreach(_in_file ${in_files})
  string(REPLACE "PRODUCTSUFFIX" "${PRODUCT_SUFFIX}" _out_file "${_in_file}")
  configure_file(
    ${CMAKE_SOURCE_DIR}/${_in_file}.in
    ${DEST_DIR}/${_out_file}
    @ONLY
  )
endforeach()

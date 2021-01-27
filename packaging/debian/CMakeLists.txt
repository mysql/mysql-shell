# Copyright (c) 2016, 2021, Oracle and/or its affiliates.
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

#
# Timestamp for use in debian/changelog
#
# Note: Debian tools use the timestamp of the last entry in the changelog
# as the value of SOURCE_DATE_EPOCH env. variable which fixes build time
# at that point. This will, for example, fix the build date reported in
# in INFO_BIN file. Here we determine the value of the timestamp of the
# last changelog entry generated from changelog.in below.
#

IF (NOT DEFINED DEB_CHANGELOG_TIMESTAMP)
  execute_process(
    COMMAND date --rfc-2822
    OUTPUT_VARIABLE DEB_CHANGELOG_TIMESTAMP
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  SET (DEB_CHANGELOG_TIMESTAMP ${DEB_CHANGELOG_TIMESTAMP} CACHE STRING "")
ENDIF()

# A string(TIMESTAMP YEAR "%Y") would do but as using "date" above, use the same
IF (NOT DEFINED DEB_COPYRIGHT_YEAR)
  execute_process(
    COMMAND date +%Y
    OUTPUT_VARIABLE DEB_COPYRIGHT_YEAR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  SET (DEB_COPYRIGHT_YEAR ${DEB_COPYRIGHT_YEAR} CACHE STRING "")
ENDIF()

# We make sure it loads only the protobuf version
set(ONLY_PROTOBUF_VERSION 1)
include(../../cmake/protobuf.cmake)

set(PRODUCT   "MySQL Shell (part of MySQL Server) ${MYSH_BASE_VERSION}")

if(NOT DEBIAN_REVISION)
  set(DEBIAN_REVISION 1)
endif()

# Distinguish between community and non-community builds, with the
# default being a community build. This does not impact the feature
# set that will be compiled in; it's merely provided as a hint to
# custom packaging steps.
option(COMMUNITY_BUILD "Set to true if this is a community build" ON)

if(NOT COMMUNITY_BUILD)
  set(PRODUCT_SUFFIX "-commercial")
  set(CONFLICTING_PRODUCT_SUFFIX "")
  set(VERSION "${MYSH_VERSION}+commercial-${DEBIAN_REVISION}")
else()
  set(PRODUCT_SUFFIX "")
  set(CONFLICTING_PRODUCT_SUFFIX "-commercial")
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
set(DEB_DEPS)

if(WITH_SSL)
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_SSL=${WITH_SSL}")
else()
  list(APPEND DEB_BUILD_DEPS "openssl")
endif()

if(WITH_CURL)
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_CURL=${WITH_CURL}")
else()
  list(APPEND DEB_BUILD_DEPS "libcurl4-openssl-dev | libcurl-dev")
endif()

if(MYSQLCLIENT_STATIC_LINKING OR ENV{MYSQLCLIENT_STATIC_LINKING})
  list(APPEND EXTRA_CMAKE_OPTS "-DMYSQLCLIENT_STATIC_LINKING=ON")
endif()

if(NOT MYSQL_DIR AND NOT MYSQL_INCLUDE_DIR AND NOT MYSQL_LIB_DIR AND
   "$ENV{MYSQL_DIR}" AND NOT "$ENV{MYSQL_INCLUDE_DIR}" AND NOT "$ENV{MYSQL_LIB_DIR}")
  list(APPEND DEB_BUILD_DEPS "libmysqlclient-dev")
endif()

if(WITH_PROTOBUF)
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_PROTOBUF=${WITH_PROTOBUF}")
else()
  if (MYSQL_SOURCE_DIR AND MYSQL_BUILD_DIR)
    list(APPEND EXTRA_CMAKE_OPTS "-DPROTOBUF_INCLUDE_DIR=${MYSQL_SOURCE_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/src")
    list(APPEND EXTRA_CMAKE_OPTS "-DPROTOBUF_LIBRARY=${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/cmake/libprotobuf.a")
    list(APPEND EXTRA_CMAKE_OPTS "-DPROTOBUF_LIBRARY_DEBUG=${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/cmake/libprotobuf.a")
  else()
    list(APPEND DEB_BUILD_DEPS "libprotobuf-dev")
    list(APPEND DEB_BUILD_DEPS "protobuf-compiler")
  endif()
endif()

if(V8_INCLUDE_DIR)
  list(APPEND EXTRA_CMAKE_OPTS "-DV8_INCLUDE_DIR=${V8_INCLUDE_DIR}")
endif()
if(V8_LIB_DIR)
  list(APPEND EXTRA_CMAKE_OPTS "-DV8_LIB_DIR=${V8_LIB_DIR}")
endif()

if(HAVE_PYTHON)
  list(APPEND EXTRA_CMAKE_OPTS "-DHAVE_PYTHON=1")
  if (BUNDLED_PYTHON_DIR)
    list(APPEND EXTRA_CMAKE_OPTS "-DBUNDLED_PYTHON_DIR=${BUNDLED_PYTHON_DIR}")
  else()
    list(APPEND DEB_BUILD_DEPS "python3-dev (>= 3.6)")
    # Most runtime deps the Deb build tools figures out, like the dependency
    # on "libpythonVER.so". But it can't know about the dependency on the
    # standard Python modules, i.e. almost all of a normal Python install
    list(APPEND DEB_DEPS "python3 (>= 3.6)")
  endif()
endif()

if(MYSQL_SOURCE_DIR)
  list(APPEND EXTRA_CMAKE_OPTS "-DMYSQL_SOURCE_DIR=${MYSQL_SOURCE_DIR} -DMYSQL_BUILD_DIR=${MYSQL_SOURCE_DIR}/bld")
endif()

if(WITH_GMOCK)
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_TESTS=ON")
  list(APPEND EXTRA_CMAKE_OPTS "-DWITH_GMOCK=${WITH_GMOCK}")
endif()

if(PYTHON_DEPS)
  list(APPEND EXTRA_CMAKE_OPTS "-DPYTHON_DEPS=${PYTHON_DEPS}")
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
)

foreach(_in_file ${in_files})
  string(REPLACE "PRODUCTSUFFIX" "${PRODUCT_SUFFIX}" _out_file "${_in_file}")
  configure_file(
    ${CMAKE_SOURCE_DIR}/${_in_file}.in
    ${DEST_DIR}/${_out_file}
    @ONLY
  )
endforeach()

# We need mysql-shell.install file to be dynamic depending on BUNDLED_PYTHON_DIR
# option, and configure_file() does not have control statements, therefore we
# are creating this file in following way.
file(WRITE "${DEST_DIR}/mysql-shell${PRODUCT_SUFFIX}.install"
"usr/bin/mysqlsh
usr/bin/mysql-secret-store-login-path
usr/share/mysqlsh/LICENSE    usr/share/doc/mysqlsh
usr/share/mysqlsh/README     usr/share/doc/mysqlsh
usr/share/mysqlsh/Docs/INFO_SRC     usr/share/mysqlsh/Docs
usr/share/mysqlsh/Docs/INFO_BIN     usr/share/mysqlsh/Docs
usr/share/mysqlsh/prompt/*   usr/share/mysqlsh/prompt
usr/share/mysqlsh/upgrade_checker.msg
usr/lib/mysqlsh/plugins
usr/lib/mysqlsh/python-packages
")

if (BUNDLED_PYTHON_DIR OR PYTHON_DEPS)
file(APPEND "${DEST_DIR}/mysql-shell${PRODUCT_SUFFIX}.install"
"usr/lib/mysqlsh/lib/*   usr/lib/mysqlsh/lib
")
endif()

if (BUNDLED_PYTHON_DIR)
file(APPEND "${DEST_DIR}/mysql-shell${PRODUCT_SUFFIX}.install"
"# For python bundle
usr/lib/mysqlsh/include
usr/lib/mysqlsh/libpython*.so*")
endif()

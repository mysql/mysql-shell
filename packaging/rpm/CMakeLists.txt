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
project(mysh_rpm_init NONE)

# ----------------------------------------------------------------------
# Set some variables to replace
# Use the version variables from "version.cmake"
# ----------------------------------------------------------------------

SET(ROOT_PROJECT_DIR "${CMAKE_SOURCE_DIR}/../../")
include(../../version.cmake)

# We make sure it loads only the protobuf version
set(ONLY_PROTOBUF_VERSION 1)
include(../../cmake/protobuf.cmake)

#set(YEAR      2016)  # FIXME automate somehow
set(PRODUCT   "MySQL Shell (part of MySQL Server) ${MYSH_BASE_VERSION}")

# User can specify the RPM_RELEASE string but if not given, create one
if(NOT RPM_RELEASE)
  if(EXTRA_VERSION)   # -rc, -dmr, ...., and assume non GA
    # Extract "rc", "dmr" ...
    set(_rpm_extra_version "${EXTRA_VERSION}")
    string(REGEX REPLACE "^[^A-Za-z0-9]" "" _rpm_extra_version "${_rpm_extra_version}")
    string(REGEX REPLACE "[^A-Za-z0-9]" "_" _rpm_extra_version "${_rpm_extra_version}")
    set(RPM_RELEASE "0.1.${_rpm_extra_version}")
  else()
    set(RPM_RELEASE "1")
  endif()
endif()


# Distinguish between community and non-community builds, with the
# default being a community build. This does not impact the feature
# set that will be compiled in; it's merely provided as a hint to
# custom packaging steps.
option(COMMUNITY_BUILD "Set to true if this is a community build" ON)

# Distinguish cloud builds from commercial/gpl
option(CLOUD_BUILD "Set to true if this is a cloud build" OFF)

set(CLOUD_VER "0")
if (CLOUD_BUILD)
  set(PRODUCT_SUFFIX "-cloud")
  set(LICENSE_TYPE   "Cloud")
  set(COMMERCIAL_VER     "0")
  set(CLOUD_VER          "1")
  # We add ".2" to RPM release if commercial
  set(RPM_RELEASE "${RPM_RELEASE}.2")
else()
  if(NOT COMMUNITY_BUILD)
    set(PRODUCT_SUFFIX "-commercial")
    set(LICENSE_TYPE   "Commercial")
    set(COMMERCIAL_VER     "1")
    # We add ".1" to RPM release if commercial
    set(RPM_RELEASE "${RPM_RELEASE}.1")
  else()
    set(PRODUCT_SUFFIX "")
    set(LICENSE_TYPE   "GPLv2")
  endif()
endif()

# ----------------------------------------------------------------------
# Process and copy the spec file to the top directory
# ----------------------------------------------------------------------

set(DEST_DIR ${CMAKE_SOURCE_DIR}/../../)

file(
  COPY ${CMAKE_SOURCE_DIR}/
  DESTINATION ${DEST_DIR}
  PATTERN "*.in"   EXCLUDE
  PATTERN "CMake*" EXCLUDE
)

set(in_files
  mysql-shell.spec
)

foreach(_in_file ${in_files})
  configure_file(
    ${CMAKE_SOURCE_DIR}/${_in_file}.in
    ${DEST_DIR}/${_in_file}
    @ONLY
  )
endforeach()

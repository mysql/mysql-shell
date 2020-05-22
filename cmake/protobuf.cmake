# Copyright (c) 2015, 2020, Oracle and/or its affiliates. All rights reserved.
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
# Location where to look for compiled protobuf library. It can be location of binary
# distribution or location where protobuf was built from sources. If not given, system
# default locations will be searched.
#

SET(PROTOBUF_VERSION "3.11.4")
SET(WITH_PROTOBUF $ENV{WITH_PROTOBUF} CACHE PATH "Protobuf location")

IF(WITH_PROTOBUF)

  IF(MSVC AND EXISTS ${WITH_PROTOBUF}/vsprojects)

    IF(NOT PROTOBUF_SRC_ROOT_FOLDER)
      SET(PROTOBUF_SRC_ROOT_FOLDER "${WITH_PROTOBUF}")
    ENDIF()

    FIND_PROGRAM(PROTOBUF_PROTOC_EXECUTABLE
      NAMES protoc
      DOC "The Google Protocol Buffers Compiler"
      PATHS ${WITH_PROTOBUF}/bin
      NO_DEFAULT_PATH
    )

  ELSE()

    FIND_PATH(PROTOBUF_INCLUDE_DIR
      google/protobuf/service.h
      PATH ${WITH_PROTOBUF}/include
      NO_DEFAULT_PATH
    )

    # Set prefix
    IF(MSVC)
      SET(PROTOBUF_ORIG_FIND_LIBRARY_PREFIXES "${CMAKE_FIND_LIBRARY_PREFIXES}")
      SET(CMAKE_FIND_LIBRARY_PREFIXES "lib" "")
    ENDIF()

    IF(WITH_STATIC_LINKING)
      SET(ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
      IF(NOT WIN32)
        SET(CMAKE_FIND_LIBRARY_SUFFIXES .a)
      ELSE()
        SET(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
      ENDIF()
    ENDIF()

    FIND_LIBRARY(PROTOBUF_LIBRARY
      NAMES protobuf
      PATHS ${WITH_PROTOBUF}/lib ${WITH_PROTOBUF}/lib/sparcv9 ${WITH_PROTOBUF}/lib/amd64
      NO_DEFAULT_PATH
    )

    IF(WITH_STATIC_LINKING)
      SET(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
    ENDIF()

    # Restore original find library prefixes
    IF(MSVC)
      SET(CMAKE_FIND_LIBRARY_PREFIXES "${PROTOBUF_ORIG_FIND_LIBRARY_PREFIXES}")
    ENDIF()

  ENDIF()
ELSE()
  IF (MYSQL_SOURCE_DIR AND MYSQL_BUILD_DIR)

    FIND_PROGRAM(PROTOBUF_PROTOC_EXECUTABLE
      NAMES protoc
      DOC "The Google Protocol Buffers Compiler"
      PATHS ${MYSQL_BUILD_DIR}/bin
      NO_DEFAULT_PATH
    )

    SET(PROTOBUF_INCLUDE_DIR "${MYSQL_SOURCE_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/src")
    IF (WIN32)
      IF(CMAKE_BUILD_TYPE STREQUAL Debug)
        SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/cmake/${CMAKE_BUILD_TYPE}/libprotobufd.lib")
        SET(PROTOBUF_LIBRARY_DEBUG "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/cmake/${CMAKE_BUILD_TYPE}/libprotobufd.lib")
      ELSE()
        SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/cmake/${CMAKE_BUILD_TYPE}/libprotobuf.lib")
      ENDIF()
    ELSE()
      IF(CMAKE_BUILD_TYPE STREQUAL Debug)
        SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/cmake/libprotobufd.a")
        SET(PROTOBUF_LIBRARY_DEBUG "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/cmake/libprotobufd.a")
      ELSE()
        SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_VERSION}/cmake/libprotobuf.a")
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()

# Protobuf will only be required if ONLY_PROTOBUF_VERSION is not defined
if (NOT ONLY_PROTOBUF_VERSION)
  FIND_PACKAGE(Protobuf "${PROTOBUF_VERSION}" REQUIRED)

  IF(NOT PROTOBUF_FOUND)
    MESSAGE(FATAL_ERROR "Protobuf could not be found")
  ENDIF()

  MESSAGE("PROTOBUF_INCLUDE_DIRS: ${PROTOBUF_INCLUDE_DIRS}")
  MESSAGE("PROTOBUF_LIBRARIES: ${PROTOBUF_LIBRARIES}")
ENDIF()

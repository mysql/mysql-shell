# Copyright (c) 2015, 2024, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
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

SET(PROTOBUF_VERSION "4.24.4")
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

    SET(_protobuf_protoc_dir "${MYSQL_BUILD_DIR}/runtime_output_directory")

    IF(WIN32)
      STRING(APPEND _protobuf_protoc_dir "/${CMAKE_BUILD_TYPE}")
    ENDIF()

    FIND_PROGRAM(PROTOBUF_PROTOC_EXECUTABLE
      NAMES protoc
      DOC "The Google Protocol Buffers Compiler"
      PATHS "${_protobuf_protoc_dir}/bin"
      NO_DEFAULT_PATH
    )

    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" PROTOBUF_VERSION_MATCH "${PROTOBUF_VERSION}")
    SET(PROTOBUF_VERSION_MAJOR "${CMAKE_MATCH_1}")
    SET(PROTOBUF_VERSION_MINOR "${CMAKE_MATCH_2}")
    SET(PROTOBUF_VERSION_PATCH "${CMAKE_MATCH_3}")

    SET(PROTOBUF_BUNDLED_VERSION "${PROTOBUF_VERSION_MINOR}.${PROTOBUF_VERSION_PATCH}")

    SET(PROTOBUF_INCLUDE_DIR "${MYSQL_SOURCE_DIR}/extra/protobuf/protobuf-${PROTOBUF_BUNDLED_VERSION}/src")
    IF (WIN32)
      IF(CMAKE_BUILD_TYPE STREQUAL Debug)
        SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_BUNDLED_VERSION}/${CMAKE_BUILD_TYPE}/libprotobufd.lib")
      ELSE()
        SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_BUNDLED_VERSION}/${CMAKE_BUILD_TYPE}/libprotobuf.lib")
      ENDIF()
    ELSE()
      IF(CMAKE_BUILD_TYPE STREQUAL Debug)
        SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_BUNDLED_VERSION}/libprotobufd.a")
      ELSE()
        SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_BUNDLED_VERSION}/libprotobuf.a")
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

  IF(NOT WITH_PROTOBUF)
    SET(ABSEIL_DIR "extra/abseil/abseil-cpp-20230802.1")
    list(APPEND PROTOBUF_INCLUDE_DIRS "${MYSQL_SOURCE_DIR}/${ABSEIL_DIR}")

    IF(WIN32)
      list(APPEND PROTOBUF_LIBRARIES "${MYSQL_BUILD_DIR}/${ABSEIL_DIR}/absl/${CMAKE_BUILD_TYPE}/abseil_dll.lib")
      list(APPEND PROTOBUF_LIBRARIES "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_BUNDLED_VERSION}/third_party/utf8_range/${CMAKE_BUILD_TYPE}/utf8_validity.lib")
      SET(BUNDLED_ABSEIL_LIBRARY "${MYSQL_BUILD_DIR}/library_output_directory/${CMAKE_BUILD_TYPE}/abseil_dll.dll")
    ELSE()
      FILE(GLOB_RECURSE _absl_libs LIST_DIRECTORIES false "${MYSQL_BUILD_DIR}/${ABSEIL_DIR}/absl/*.a")

      IF(NOT APPLE)
        list(APPEND PROTOBUF_LIBRARIES "-Wl,--start-group")
      ENDIF()

      list(APPEND PROTOBUF_LIBRARIES ${_absl_libs})
      list(APPEND PROTOBUF_LIBRARIES "${MYSQL_BUILD_DIR}/extra/protobuf/protobuf-${PROTOBUF_BUNDLED_VERSION}/third_party/utf8_range/libutf8_validity.a")

      IF(NOT APPLE)
        list(APPEND PROTOBUF_LIBRARIES "-Wl,--end-group")
      ENDIF()

      IF(APPLE)
        list(APPEND PROTOBUF_LIBRARIES "-Wl,-framework,CoreFoundation")
      ENDIF()
    ENDIF()
  ENDIF()

  MESSAGE("PROTOBUF_INCLUDE_DIRS: ${PROTOBUF_INCLUDE_DIRS}")
  MESSAGE("PROTOBUF_LIBRARIES: ${PROTOBUF_LIBRARIES}")
ENDIF()

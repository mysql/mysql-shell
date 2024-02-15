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

IF(WITH_PROTOBUF)
  IF(NOT WITH_PROTOBUF STREQUAL "system")
    MESSAGE(FATAL_ERROR "The only supported value for WITH_PROTOBUF is 'system'")
  ENDIF()

  # Lowest checked system version is 3.5.0 on Oracle Linux 8.
  # Older versions may generate code which breaks the -Werror build.
  SET(PROTOBUF_VERSION "3.5.0")
ELSE()
  IF(NOT DEFINED WITH_PROTOBUF_LITE AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    # if not set explicitly, non-Debug builds use protobuf-lite
    SET(WITH_PROTOBUF_LITE ON)
  ENDIF()

  FIND_PROGRAM(PROTOBUF_PROTOC_EXECUTABLE
    NAMES protoc
    DOC "The Google Protocol Buffers Compiler"
    PATHS ${MYSQL_BUILD_DIR}/bin
    NO_DEFAULT_PATH
  )

  string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" PROTOBUF_VERSION_MATCH "${PROTOBUF_VERSION}")
  SET(PROTOBUF_VERSION_MAJOR "${CMAKE_MATCH_1}")
  SET(PROTOBUF_VERSION_MINOR "${CMAKE_MATCH_2}")
  SET(PROTOBUF_VERSION_PATCH "${CMAKE_MATCH_3}")

  SET(PROTOBUF_BUNDLED_VERSION "${PROTOBUF_VERSION_MINOR}.${PROTOBUF_VERSION_PATCH}")
  SET(PROTOBUF_SO_VERSION "${PROTOBUF_VERSION_MINOR}.${PROTOBUF_VERSION_PATCH}.0")

  SET(PROTOBUF_INCLUDE_DIR "${MYSQL_SOURCE_DIR}/extra/protobuf/protobuf-${PROTOBUF_BUNDLED_VERSION}/src")

  SET(_protobuf_lib_dir "${MYSQL_BUILD_DIR}/library_output_directory")

  IF(WIN32)
    STRING(APPEND _protobuf_lib_dir "/${CMAKE_BUILD_TYPE}")
  ENDIF()

  IF(WIN32)
    SET(_protobuf_lib_ext "dll")
  ELSEIF(APPLE)
    SET(_protobuf_lib_ext "${PROTOBUF_SO_VERSION}.dylib")
  ELSE()
    SET(_protobuf_lib_ext "so.${PROTOBUF_SO_VERSION}")
  ENDIF()

  IF(WITH_PROTOBUF_LITE)
    SET(_protobuf_lib_suffix "-lite")
  ENDIF()

  SET(PROTOBUF_LIBRARY "${_protobuf_lib_dir}/libprotobuf${_protobuf_lib_suffix}.${_protobuf_lib_ext}")
  SET(BUNDLED_PROTOBUF_LIBRARY "${PROTOBUF_LIBRARY}")
  get_filename_component(BUNDLED_PROTOBUF_LIBRARY_NAME "${BUNDLED_PROTOBUF_LIBRARY}" NAME)

  IF(WIN32)
    # on Windows we need to link with the .lib file
    SET(PROTOBUF_LIBRARY "${MYSQL_BUILD_DIR}/bin/${CMAKE_BUILD_TYPE}/libprotobuf${_protobuf_lib_suffix}.lib")
  ENDIF()

  IF(NOT EXISTS "${PROTOBUF_LIBRARY}")
    MESSAGE(FATAL_ERROR "Could not find Protobuf library: ${PROTOBUF_LIBRARY}")
  ENDIF()
  IF(NOT EXISTS "${BUNDLED_PROTOBUF_LIBRARY}")
    MESSAGE(FATAL_ERROR "Could not find bundled Protobuf library: ${BUNDLED_PROTOBUF_LIBRARY}")
  ENDIF()
ENDIF()

FIND_PACKAGE(Protobuf "${PROTOBUF_VERSION}" REQUIRED)

IF(NOT WITH_PROTOBUF)
  SET(ABSEIL_DIR "extra/abseil/abseil-cpp-20230802.1")
  list(APPEND PROTOBUF_INCLUDE_DIRS "${MYSQL_SOURCE_DIR}/${ABSEIL_DIR}")

  IF(WIN32)
    list(APPEND PROTOBUF_LIBRARIES "${MYSQL_BUILD_DIR}/${ABSEIL_DIR}/absl/${CMAKE_BUILD_TYPE}/abseil_dll.lib")
    SET(BUNDLED_ABSEIL_LIBRARIES "${_protobuf_lib_dir}/abseil_dll.${_protobuf_lib_ext}")
  ELSE()
    FILE(GLOB BUNDLED_ABSEIL_LIBRARIES LIST_DIRECTORIES false "${_protobuf_lib_dir}/libabsl_*.so")

    IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
      # we need to explicitly link with log libraries
      FOREACH(_lib ${BUNDLED_ABSEIL_LIBRARIES})
        IF(_lib MATCHES "libabsl_log_")
          list(APPEND PROTOBUF_LIBRARIES "${_lib}")
        ENDIF()
      ENDFOREACH()
    ENDIF()
  ENDIF()
ENDIF()

MESSAGE("PROTOBUF_INCLUDE_DIRS: ${PROTOBUF_INCLUDE_DIRS}")
MESSAGE("PROTOBUF_LIBRARIES: ${PROTOBUF_LIBRARIES}")

# Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


#
# Location where to look for compiled protobuf library. It can be location of binary
# distribution or location where protobuf was built from sources. If not given, system
# default locations will be searched.
#

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

ENDIF()

FIND_PACKAGE(Protobuf "2.6.0" REQUIRED)

IF(NOT PROTOBUF_FOUND)
  MESSAGE(FATAL_ERROR "Protobuf could not be found")
ENDIF()

MESSAGE("PROTOBUF_INCLUDE_DIRS: ${PROTOBUF_INCLUDE_DIRS}")
MESSAGE("PROTOBUF_LIBRARIES: ${PROTOBUF_LIBRARIES}")

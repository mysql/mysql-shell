# Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

IF(NOT BUILD_SOURCE_PACKAGE AND NOT CURL_LIBRARY)
  MESSAGE(FATAL_ERROR "CURL library not found!")
ENDIF()

IF(OPENSSL_INCLUDE_DIR)
  INCLUDE_DIRECTORIES(BEFORE ${OPENSSL_INCLUDE_DIR})
ENDIF()

set(rest_SOURCES
  authentication.cc
  error_codes.cc
  headers.cc
  rest_service.cc
  response.cc
  retry_strategy.cc
  signed_rest_service.cc
)

add_convenience_library(rest
  ${rest_SOURCES}
  COMPILE_DEFINITIONS "${CURL_DEFINITIONS}"
  INCLUDE_DIRECTORIES ${TINYXML2_INCLUDE_DIRECTORIES}
)

include_directories(SYSTEM ${CURL_INCLUDE_DIR})
target_link_libraries(rest ssh ${CURL_LIBRARY})

if (WIN32)
  target_link_libraries(rest Crypt32)
endif()

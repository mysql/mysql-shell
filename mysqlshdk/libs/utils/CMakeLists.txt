# Copyright (c) 2014, 2020, Oracle and/or its affiliates. All rights reserved.
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

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

IF(OPENSSL_INCLUDE_DIR)
    INCLUDE_DIRECTORIES(BEFORE ${OPENSSL_INCLUDE_DIR})
ENDIF()

set(utils_SOURCE
    array_result.cc
    utils_mysql_parsing.cc
    utils_net.cc
    utils_file.cc
    utils_path.cc
    utils_process.cc
    utils_json.cc
    utils_general.cc
    utils_sqlstring.cc
    strformat.cc
    utils_string.cc
    utils_uuid.cc
    utils_stacktrace.cc
    utils_lexing.cc
    utils_buffered_input.cc
    utils_translate.cc
    document_parser.cc
    trandom.cc
    nullable_options.cc
    base_tokenizer.cc
    logger.cc
    process_launcher.cc
    debug.cc
    options.cc
    uuid_gen.cc
    version.cc
    profiling.cc
    rate_limit.cc
    ssl_keygen.cc
)

IF(WITH_OCI)
  LIST(APPEND utils_SOURCE "oci_setup.cc")
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL Debug)
  LIST(APPEND utils_SOURCE fault_injection.cc)
ENDIF()

# platform dependent implementations
IF(UNIX)
    LIST(APPEND utils_SOURCE utils_path_unix.cc)
ELSEIF(WIN32)
    LIST(APPEND utils_SOURCE utils_path_win32.cc)
ENDIF()

add_convenience_library(utils ${utils_SOURCE})
ADD_STAN_TARGET(utils ${utils_SOURCE})

target_link_libraries(utils db)

if (WIN32)
  target_link_libraries(utils ws2_32)
else ()
  IF(CMAKE_SYSTEM_NAME MATCHES "SunOS")
    target_link_libraries(utils socket nsl)
  ELSE()
    target_link_libraries(utils util)
  ENDIF()
endif()

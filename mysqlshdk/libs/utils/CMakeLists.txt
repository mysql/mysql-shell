# Copyright (c) 2014, 2023, Oracle and/or its affiliates.
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
    base_tokenizer.cc
    bignum.cc
    debug.cc
    document_parser.cc
    dtoa.cc
    log_sql.cc
    logger.cc
    nullable_options.cc
    options.cc
    process_launcher.cc
    profiling.cc
    rate_limit.cc
    ssl_keygen.cc
    strformat.cc
    syslog.cc
    syslog_level.cc
    thread_pool.cc
    threads.cc
    trandom.cc
    utils_buffered_input.cc
    utils_encoding.cc
    utils_file.cc
    utils_general.cc
    utils_json.cc
    utils_lexing.cc
    utils_mysql_parsing.cc
    utils_net.cc
    utils_os.cc
    utils_path.cc
    utils_process.cc
    utils_sqlstring.cc
    utils_stacktrace.cc
    utils_string.cc
    utils_time.cc
    utils_translate.cc
    utils_uuid.cc
    uuid_gen.cc
    version.cc
)

IF(CMAKE_BUILD_TYPE STREQUAL Debug)
  LIST(APPEND utils_SOURCE fault_injection.cc)
ENDIF()

# platform dependent implementations
IF(UNIX)
    LIST(APPEND utils_SOURCE syslog_system_unix.cc utils_path_unix.cc)
ELSEIF(WIN32)
    LIST(APPEND utils_SOURCE utils_path_win32.cc)

    SET(SYSLOG_EVENT_MC "${CMAKE_SOURCE_DIR}/res/syslog_event.mc")
    SET(SYSLOG_EVENT_H  "${CMAKE_CURRENT_BINARY_DIR}/syslog_event.h")
    SET(SYSLOG_EVENT_RC "${CMAKE_CURRENT_BINARY_DIR}/syslog_event.rc"
        CACHE INTERNAL "Path to RC file with message resources")

    add_custom_command(
        OUTPUT
            "${SYSLOG_EVENT_H}"
            "${SYSLOG_EVENT_RC}"
            "${CMAKE_CURRENT_BINARY_DIR}/MSG00001.bin"
        COMMAND mc -c -n
            "${SYSLOG_EVENT_MC}"
            -h "${CMAKE_CURRENT_BINARY_DIR}"
            -r "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS
            "${SYSLOG_EVENT_MC}"
    )

    add_custom_target(generate_syslog_event_rc DEPENDS "${SYSLOG_EVENT_RC}")

    LIST(APPEND utils_SOURCE
        "${SYSLOG_EVENT_H}"
         syslog_system_win32.cc
    )

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

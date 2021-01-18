# Copyright (c) 2020, 2021, Oracle and/or its affiliates.
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

set(fuzz_uri_parser_SRC
    "fuzz_uri_parser.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/db/uri_parser.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/base_tokenizer.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/db/connection_options.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/nullable_options.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/utils_general.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/utils_sqlstring.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/utils_string.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/utils_file.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/utils_path.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/utils_path_unix.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/utils_lexing.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/utils_net.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/db/utils_connection.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/dtoa.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/db/ssl_options.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/db/uri_encoder.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/ssh/ssh_connection_options.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/ssh/ssh_session_options.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/ssh/ssh_common.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/storage/utils.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/logger.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/shellcore/scoped_contexts.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/shellcore/interrupt_handler.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/shellcore/sigint_event.cc"
)
add_fuzz_test("fuzz_uri_parser" "${fuzz_uri_parser_SRC}")

/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#include <array>

namespace keyword_diff_84 {

inline constexpr std::array<keyword_t, 4> added = {{
    {"AUTO", 0},
    {"BERNOULLI", 0},
    {"MANUAL", 0},
    {"TABLESAMPLE", 0},
}};

inline constexpr std::array<keyword_t, 27> removed = {{
    {"GET_MASTER_PUBLIC_KEY", 0},
    {"MASTER_AUTO_POSITION", 0},
    {"MASTER_BIND", 1},
    {"MASTER_COMPRESSION_ALGORITHMS", 0},
    {"MASTER_CONNECT_RETRY", 0},
    {"MASTER_DELAY", 0},
    {"MASTER_HEARTBEAT_PERIOD", 0},
    {"MASTER_HOST", 0},
    {"MASTER_LOG_FILE", 0},
    {"MASTER_LOG_POS", 0},
    {"MASTER_PASSWORD", 0},
    {"MASTER_PORT", 0},
    {"MASTER_PUBLIC_KEY_PATH", 0},
    {"MASTER_RETRY_COUNT", 0},
    {"MASTER_SSL", 0},
    {"MASTER_SSL_CA", 0},
    {"MASTER_SSL_CAPATH", 0},
    {"MASTER_SSL_CERT", 0},
    {"MASTER_SSL_CIPHER", 0},
    {"MASTER_SSL_CRL", 0},
    {"MASTER_SSL_CRLPATH", 0},
    {"MASTER_SSL_KEY", 0},
    {"MASTER_SSL_VERIFY_SERVER_CERT", 1},
    {"MASTER_TLS_CIPHERSUITES", 0},
    {"MASTER_TLS_VERSION", 0},
    {"MASTER_USER", 0},
    {"MASTER_ZSTD_COMPRESSION_LEVEL", 0},
}};

}  // namespace keyword_diff_84

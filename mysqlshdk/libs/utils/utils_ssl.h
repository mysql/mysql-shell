/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_SSL_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_SSL_H_

#include <string>
#include <string_view>
#include <vector>

namespace shcore {
namespace ssl {

/**
 * Converts the given hash to the fingerprint format.
 */
std::string to_fingerprint(const std::vector<unsigned char> &hash);

enum class Hash {
  SHA256,
  RESTRICTED_SHA1,
  RESTRICTED_MD5,
};

/**
 * Computes SHA256 hash of the given data.
 */
std::vector<unsigned char> sha256(std::string_view data);

std::vector<unsigned char> hmac_sha256(const std::vector<unsigned char> &key,
                                       std::string_view data);

namespace restricted {

/**
 * Computes SHA1 hash of the given data.
 *
 * @param data The given data.
 *
 * @returns SHA1 hash
 *
 * NOTE: The SHA1 hash cannot be used without an approval.
 */
std::vector<unsigned char> sha1(std::string_view data);

/**
 * Computes MD5 hash of the given data.
 *
 * @param data The given data.
 *
 * @returns MD5 hash
 *
 * NOTE: The MD5 hash cannot be used without an approval.
 */
std::vector<unsigned char> md5(std::string_view data);

}  // namespace restricted

}  // namespace ssl
}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_SSL_H_

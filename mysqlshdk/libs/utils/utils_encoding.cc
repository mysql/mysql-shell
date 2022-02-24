/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/utils/utils_encoding.h"

// #include <openssl/err.h>
#include <openssl/evp.h>
// #include <openssl/md5.h>
// #include <openssl/pem.h>
// #include <openssl/ssl.h>
// #include <algorithm>
#include <cassert>
// #include <functional>
// #include <limits>
#include <memory>
// #include <regex>
// #include <utility>
// #include <vector>

namespace shcore {
namespace {
using BIO_ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
}

bool decode_base64(const std::string &source, std::string *target) {
  assert(target);
  //--- Unencodes the Public Key Data
  size_t size = source.size();
  size_t padding =
      source[size - 2] == '=' ? 2 : source[size - 1] == '=' ? 1 : 0;

  const size_t expected_size = (size * 3) / 4 - padding;

  auto buffer = std::make_unique<char[]>(expected_size);

  BIO_ptr bio_source(BIO_new_mem_buf(source.c_str(), source.size()),
                     ::BIO_free);
  BIO_ptr bio_base64(BIO_new(BIO_f_base64()), ::BIO_free);
  BIO_push(bio_base64.get(), bio_source.get());
  BIO_set_flags(bio_base64.get(), BIO_FLAGS_BASE64_NO_NL);
  size_t final_size = BIO_read(bio_base64.get(), buffer.get(), size);

  target->assign(buffer.get(), expected_size);

  return final_size == expected_size;
}

bool encode_base64(const unsigned char *source, int source_length,
                   std::string *encoded) {
  assert(encoded);
  assert(source_length >= 0);
  // base64 encoding produces 4 bytes of output for every 3 bytes of source
  // data. Output is always padded in such way that length of output is
  // divisible by 4.
  const auto target_size = (((4 * source_length) / 3) + 3) & ~3;
  encoded->resize(target_size + 1);
  const auto length =
      EVP_EncodeBlock((unsigned char *)encoded->data(), source, source_length);
  if (target_size == static_cast<decltype((target_size))>(length)) {
    encoded->resize(length);
    return true;
  }
  return false;
}

}  // namespace shcore

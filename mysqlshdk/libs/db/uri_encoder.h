/*
 * Copyright (c) 2017, 2024 Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_DB_URI_ENCODER_H_
#define MYSQLSHDK_LIBS_DB_URI_ENCODER_H_

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "mysqlshdk/libs/db/uri_common.h"
#include "mysqlshdk/libs/utils/base_tokenizer.h"

namespace mysqlshdk {
namespace db {
namespace uri {

class Uri_encoder {
 public:
  Uri_encoder() = default;
  std::string encode_uri(const IUri_encodable &info,
                         Tokens_mask format = formats::full_no_password());
  std::string encode_scheme(const std::string &data);
  std::string encode_socket(const std::string &socket);
  std::string encode_userinfo(const std::string &data);
  std::string encode_host(const std::string &data);
  std::string encode_port(int port);
  std::string encode_port(const std::string &data);
  std::string encode_path_segment(const std::string &data,
                                  bool skip_already_encoded = true);
  std::string encode_attribute(const std::string &data);
  std::string encode_values(const std::vector<std::string> &values,
                            bool force_array = false);
  std::string encode_value(const std::string &data);
  std::string encode_query(const std::string &data);

 private:
  shcore::BaseTokenizer _tokenizer;
  std::string process(const std::string &data);
};

/**
 * Generates a percent encoded string based on RFC-3986, section 3.3 - output
 * string is encoded as a path component of an URI.
 */
std::string pctencode_path(const std::string &s);

/**
 * Generates a percent encoded string based on RFC-3986, section 3.4 - output
 * string is encoded as a value in a query component of an URI.
 */
std::string pctencode_query_value(const std::string &s);

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_URI_ENCODER_H_

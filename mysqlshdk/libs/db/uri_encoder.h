/*
 * Copyright (c) 2017, 2020 Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_URI_ENCODER_H_
#define MYSQLSHDK_LIBS_DB_URI_ENCODER_H_

#include <string>
#include <vector>
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/uri_common.h"
#include "mysqlshdk/libs/utils/base_tokenizer.h"

namespace mysqlshdk {
namespace db {
namespace uri {
class SHCORE_PUBLIC Uri_encoder {
 public:
  std::string encode_uri(const Connection_options &info,
                         Tokens_mask format = formats::full_no_password());
  std::string encode_scheme(const std::string &data);
  std::string encode_socket(const std::string &socket);
  std::string encode_userinfo(const std::string &data);
  std::string encode_host(const std::string &data);
  std::string encode_port(int port);
  std::string encode_port(const std::string &data);
  std::string encode_path_segment(const std::string &data);
  std::string encode_attribute(const std::string &data);
  std::string encode_values(const std::vector<std::string> &values,
                            bool force_array = false);
  std::string encode_value(const std::string &data);
  std::string encode_query(const std::string &data);

 private:
  shcore::BaseTokenizer _tokenizer;
  std::string process(const std::string &data);
};
}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_URI_ENCODER_H_

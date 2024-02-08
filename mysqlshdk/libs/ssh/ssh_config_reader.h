/*
 * Copyright (c) 2021, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_SSH_SSH_CONFIG_READER_H_
#define MYSQLSHDK_LIBS_SSH_SSH_CONFIG_READER_H_

#include <string>
#include <vector>

#include "mysqlshdk/libs/utils/enumset.h"

namespace mysqlshdk {
namespace ssh {

enum class Ssh_auth_methods {
  PASSWORD_AUTH,
  PUBKEY_AUTH,
  KBDINT_AUTH,
  GSSAPI_AUTH
};

struct Ssh_config_data {
  Ssh_config_data();
  mysqlshdk::utils::Enum_set<Ssh_auth_methods, Ssh_auth_methods::GSSAPI_AUTH>
      auth_methods;
};

class Ssh_config_reader final {
 public:
  Ssh_config_reader() = default;
  void read(Ssh_config_data *out_config, const std::string &target_host = "*",
            const std::string &path = "");
  ~Ssh_config_reader() = default;

 private:
  void read(Ssh_config_data *out_config, const std::string &target_host,
            const std::string &path, std::vector<std::string> *hosts);
};

}  // namespace ssh
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_SSH_SSH_CONFIG_READER_H_

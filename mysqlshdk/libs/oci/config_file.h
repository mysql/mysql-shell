/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_OCI_CONFIG_FILE_H_
#define MYSQLSHDK_LIBS_OCI_CONFIG_FILE_H_

#include <string>

#include "mysqlshdk/libs/config/config_file.h"

namespace mysqlshdk {
namespace oci {

/**
 * An OCI config file.
 */
class Config_file final {
 public:
  enum class Entry {
    USER,
    FINGERPRINT,
    KEY_FILE,
    PASS_PHRASE,
    TENANCY,
    REGION,
    SECURITY_TOKEN_FILE,
    DELEGATION_TOKEN_FILE,
  };

  Config_file(const std::string &config_file,
              const std::string &config_profile);

  Config_file(const Config_file &) = default;
  Config_file(Config_file &&) = default;

  Config_file &operator=(const Config_file &) = default;
  Config_file &operator=(Config_file &&) = default;

  ~Config_file() = default;

  inline const std::string &config_file() const noexcept {
    return m_config_file;
  }

  inline const std::string &config_profile() const noexcept {
    return m_config_profile;
  }

  std::string config_option(Entry entry);

 private:
  std::string m_config_file;
  std::string m_config_profile;

  mysqlshdk::config::Config_file m_config;
  bool m_config_is_read = false;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_CONFIG_FILE_H_

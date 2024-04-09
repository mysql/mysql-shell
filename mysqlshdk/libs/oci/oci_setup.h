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

#ifndef MYSQLSHDK_LIBS_OCI_OCI_SETUP_H_
#define MYSQLSHDK_LIBS_OCI_OCI_SETUP_H_

#include <string>

#include "mysqlshdk/libs/config/config_file.h"

namespace mysqlshdk {
namespace oci {

class Oci_setup {
 public:
  explicit Oci_setup(const std::string &oci_config_path);

  Oci_setup &operator=(const Oci_setup &other) = delete;
  Oci_setup &operator=(Oci_setup &&other) = delete;

  ~Oci_setup() = default;

  /**
   * Loads the private key of the profile.
   *
   * @param profile The name of the profile for which the primary key will be
   * loaded.
   *
   * This function will verify if the private key associated to the profile
   * requires a passphrase, if so and not stored, then it will be loaded so the
   * user can define the passphrase and keep it cached.
   */
  void load_profile(const std::string &profile);

  /**
   * Verifies if a specific profile is already configured.
   *
   * @param profile The name of the profile to be verified.
   */
  bool has_profile(const std::string &profile);

  const mysqlshdk::config::Config_file &get_cfg() const { return m_config; }

 private:
  std::string load_private_key(const std::string &path);

  mysqlshdk::config::Config_file m_config;
};

}  // namespace oci
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_OCI_OCI_SETUP_H_

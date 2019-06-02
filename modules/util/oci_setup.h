/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_OCI_SETUP_H_
#define MODULES_UTIL_OCI_SETUP_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/include/shellcore/shell_core.h"
#include "mysqlshdk/libs/config/config_file.h"
#include "mysqlshdk/shellcore/wizard.h"

namespace mysqlsh {
namespace oci {
void init(const std::shared_ptr<shcore::Shell_core> &shell);
void load_profile(const std::string &user_profile,
                  const std::shared_ptr<shcore::Shell_core> &shell);

class Oci_setup : public shcore::wizard::Wizard {
 public:
  Oci_setup();
  explicit Oci_setup(const std::string &oci_config_path);
  Oci_setup &operator=(const Oci_setup &other) = delete;
  Oci_setup &operator=(Oci_setup &&other) = delete;

  ~Oci_setup() = default;

  /**
   * Creates an OCI profile if it does not already exist.
   *
   * @param profile The name of the profile to be created.
   */
  void create_profile(const std::string &profile);

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

  std::string get_oci_path() const { return m_oci_path; }
  std::string get_oci_cfg_path() const { return m_oci_cfg_path; }

 private:
  void init_create_profile_wizard();
  std::string load_private_key(const std::string &path,
                               const std::string &inital_prompt);

  mysqlshdk::config::Config_file m_config;
  const std::string m_oci_path;
  const std::string m_oci_cfg_path;
};

}  // namespace oci
}  // namespace mysqlsh

#endif  // MODULES_UTIL_OCI_SETUP_H_

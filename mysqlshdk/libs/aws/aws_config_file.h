/*
 * Copyright (c) 2022, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_AWS_AWS_CONFIG_FILE_H_
#define MYSQLSHDK_LIBS_AWS_AWS_CONFIG_FILE_H_

#include <optional>
#include <string>

#include "mysqlshdk/libs/aws/aws_profiles.h"

namespace mysqlshdk {
namespace aws {

/**
 * Handles both credentials and config files.
 */
class Aws_config_file final {
 public:
  Aws_config_file() = delete;

  explicit Aws_config_file(const std::string &path);

  Aws_config_file(const Aws_config_file &) = default;
  Aws_config_file(Aws_config_file &&) = default;

  Aws_config_file &operator=(const Aws_config_file &) = default;
  Aws_config_file &operator=(Aws_config_file &&) = default;

  ~Aws_config_file() = default;

  /**
   * Provides path to this file.
   */
  const std::string &path() const noexcept { return m_path; }

  /**
   * Loads the configuration from the file.
   *
   * @param require_profile_prefix If true, only 'default' section and sections
   * prefixed with 'profile ' are returned.
   *
   * @returns Loaded profiles, if the requested file exists
   *
   * @throws std::runtime_error if file is malformed
   */
  std::optional<Profiles> load(bool require_profile_prefix) const;

 private:
  std::string m_path;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_AWS_CONFIG_FILE_H_

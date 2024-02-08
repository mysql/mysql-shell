/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_AWS_PROCESS_CREDENTIALS_PROVIDER_H_
#define MYSQLSHDK_LIBS_AWS_PROCESS_CREDENTIALS_PROVIDER_H_

#include <string>

#include "mysqlshdk/libs/aws/aws_config_file.h"
#include "mysqlshdk/libs/aws/aws_credentials_provider.h"

namespace mysqlshdk {
namespace aws {

class Process_credentials_provider : public Aws_credentials_provider {
 public:
  Process_credentials_provider(const std::string &path,
                               const Aws_config_file::Profile *profile);

  Process_credentials_provider(const Process_credentials_provider &) = delete;
  Process_credentials_provider(Process_credentials_provider &&) = default;

  Process_credentials_provider &operator=(
      const Process_credentials_provider &) = delete;
  Process_credentials_provider &operator=(Process_credentials_provider &&) =
      default;

  ~Process_credentials_provider() override = default;

  static bool available(const Aws_config_file::Profile &profile);

 private:
  Credentials fetch_credentials() override;

  Credentials parse_json(const std::string &json) const;

  const Aws_config_file::Profile *m_profile;
};

}  // namespace aws
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_AWS_PROCESS_CREDENTIALS_PROVIDER_H_

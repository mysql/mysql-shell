/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/aws/s3_bucket_options.h"

#include <memory>
#include <stdexcept>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/storage/utils.h"

#include "mysqlshdk/libs/aws/s3_bucket_config.h"

namespace mysqlshdk {
namespace aws {

const shcore::Option_pack_def<S3_bucket_options> &S3_bucket_options::options() {
  static const auto opts =
      shcore::Option_pack_def<S3_bucket_options>()
          .optional(bucket_name_option(), &S3_bucket_options::m_bucket_name)
          .optional(credentials_file_option(),
                    &S3_bucket_options::m_credentials_file)
          .optional(config_file_option(), &S3_bucket_options::m_config_file)
          .optional(profile_option(), &S3_bucket_options::m_config_profile)
          .optional(endpoint_override_option(),
                    &S3_bucket_options::m_endpoint_override)
          .on_done(&S3_bucket_options::on_unpacked_options);

  return opts;
}

void S3_bucket_options::on_unpacked_options() const {
  Bucket_options::on_unpacked_options();

  if (m_bucket_name.empty()) {
    if (!m_credentials_file.empty()) {
      throw std::invalid_argument(shcore::str_format(
          s_option_error, credentials_file_option(), bucket_name_option()));
    }

    if (!m_endpoint_override.empty()) {
      throw std::invalid_argument(shcore::str_format(
          s_option_error, endpoint_override_option(), bucket_name_option()));
    }
  }

  if (!m_endpoint_override.empty()) {
    using storage::utils::get_scheme;
    using storage::utils::scheme_matches;

    const auto scheme = get_scheme(m_endpoint_override);

    if (scheme.empty()) {
      throw std::invalid_argument(
          shcore::str_format("The value of the option '%s' is missing a "
                             "scheme, expected: http:// or https://.",
                             endpoint_override_option()));
    }

    if (!scheme_matches(scheme, "http") && !scheme_matches(scheme, "https")) {
      throw std::invalid_argument(
          shcore::str_format("The value of the option '%s' uses an invalid "
                             "scheme '%s://', expected: http:// or https://.",
                             endpoint_override_option(), scheme.c_str()));
    }
  }
}

std::shared_ptr<S3_bucket_config> S3_bucket_options::s3_config() const {
  return std::make_shared<S3_bucket_config>(*this);
}

std::shared_ptr<storage::backend::object_storage::Config>
S3_bucket_options::create_config() const {
  return s3_config();
}

}  // namespace aws
}  // namespace mysqlshdk

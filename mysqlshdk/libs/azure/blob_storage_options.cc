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

#include "mysqlshdk/libs/azure/blob_storage_options.h"

#include <memory>
#include <stdexcept>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/libs/storage/utils.h"

#include "mysqlshdk/libs/azure/blob_storage_config.h"

namespace mysqlshdk {
namespace azure {

const shcore::Option_pack_def<Blob_storage_options>
    &Blob_storage_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Blob_storage_options>()
          .optional(container_name_option(),
                    &Blob_storage_options::m_container_name)
          .optional(config_file_option(), &Blob_storage_options::m_config_file)
          .optional(storage_account_option(),
                    &Blob_storage_options::m_storage_account)
          .optional(storage_sas_token_option(),
                    &Blob_storage_options::m_storage_sas_token)
          .on_done(&Blob_storage_options::on_unpacked_options);

  return opts;
}

std::shared_ptr<Blob_storage_config> Blob_storage_options::azure_config()
    const {
  return std::make_shared<Blob_storage_config>(*this);
}

std::shared_ptr<storage::backend::object_storage::Config>
Blob_storage_options::create_config() const {
  return azure_config();
}

std::vector<const char *> Blob_storage_options::get_secondary_options() const {
  return {storage_account_option(), storage_sas_token_option(),
          config_file_option()};
}

bool Blob_storage_options::has_value(const char *option) const {
  if (shcore::str_caseeq(option, container_name_option())) {
    return !m_container_name.empty();
  } else if (shcore::str_caseeq(option, config_file_option())) {
    return !m_config_file.empty();
  } else if (shcore::str_caseeq(option, storage_account_option())) {
    return !m_storage_account.empty();
  } else if (shcore::str_caseeq(option, storage_sas_token_option())) {
    return !m_storage_sas_token.empty();
  }
  return false;
}

}  // namespace azure
}  // namespace mysqlshdk

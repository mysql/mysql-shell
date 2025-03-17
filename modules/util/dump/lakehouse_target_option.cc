/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#include "modules/util/dump/lakehouse_target_option.h"

#include <memory>
#include <string>

#include "mysqlshdk/libs/aws/s3_bucket_options.h"
#include "mysqlshdk/libs/azure/blob_storage_options.h"

namespace mysqlsh {
namespace dump {

Lakehouse_target_option::Lakehouse_target::Lakehouse_target()
    : Common_options({"lakehouseTarget", "dumping vector store data to a URL",
                      false, false, true}) {}

const shcore::Option_pack_def<Lakehouse_target_option>
    &Lakehouse_target_option::options() {
  static const auto opts =
      shcore::Option_pack_def<Lakehouse_target_option>()
          .include(&Lakehouse_target_option::m_storage_options)
          .required("outputUrl", &Lakehouse_target_option::m_url)
          .template ignore<mysqlshdk::aws::S3_bucket_options>()
          .template ignore<mysqlshdk::azure::Blob_storage_options>()
          .ignore({"showProgress"})
          .on_done(&Lakehouse_target_option::on_unpacked_options);

  return opts;
}

std::unique_ptr<mysqlshdk::storage::IDirectory>
Lakehouse_target_option::output() const {
  return m_storage_options.make_directory(output_url());
}

std::string Lakehouse_target_option::describe_output_url() const {
  return m_storage_options.describe(output_url());
}

void Lakehouse_target_option::on_unpacked_options() {
  m_storage_options.set_url(m_url);
}

}  // namespace dump
}  // namespace mysqlsh

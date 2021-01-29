/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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
#include "unittest/test_utils/mocks/modules/adminapi/common/mock_metadata_storage.h"

namespace testing {
std::shared_ptr<mysqlsh::dba::Instance>
Mock_metadata_storage::def_get_md_server() {
  return mysqlsh::dba::MetadataStorage::get_md_server();
}

Mock_metadata_storage::Mock_metadata_storage(
    const std::shared_ptr<mysqlsh::dba::Instance> &instance)
    : MetadataStorage(instance) {
  ON_CALL(*this, get_md_server())
      .WillByDefault(Invoke(this, &Mock_metadata_storage::def_get_md_server));
}
}  // namespace testing

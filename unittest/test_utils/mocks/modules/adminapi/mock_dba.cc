/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "unittest/test_utils/mocks/modules/adminapi/mock_dba.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "unittest/test_utils.h"

namespace testing {
void Mock_dba::initialize(shcore::IShell_core *owner, bool chain_dba) {
  set_owner(owner);

  std::shared_ptr<StrictMock<Mock_metadata_storage>> metadata(
      &_mock_metadata, SharedDoNotDelete());
  _metadata_storage =
      std::dynamic_pointer_cast<mysqlsh::dba::MetadataStorage>(metadata);

  if (chain_dba) {
    ON_CALL(*this, call(_, _))
        .WillByDefault(Invoke(this, &mysqlsh::dba::Dba::call));
  }
}

void Mock_dba::expect_check_instance_configuration(const std::string &uri,
                                                   const std::string &status) {
  auto data = shcore::get_connection_options(uri);
  shcore::Value::Map_type_ref ret_val(new shcore::Value::Map_type);
  (*ret_val)["status"] = shcore::Value(status);
  EXPECT_CALL(*this, _check_instance_configuration(data, _, false))
      .WillOnce(Return(ret_val));
}

void Mock_dba::expect_drop_metadata_schema(
    const nullable<bool> &force, const nullable<bool> &clean_read_only,
    bool succeed) {
  shcore::Argument_list args;
  auto options = shcore::Value::new_map();
  auto map = options.as_map();

  if (!force.is_null())
    (*map)["force"] = shcore::Value(*force);

  if (!clean_read_only.is_null())
    (*map)["clearReadOnly"] = shcore::Value(*clean_read_only);

  args.push_back(options);

  if (succeed)
    EXPECT_CALL(*this, call("dropMetadataSchema", args))
        .WillOnce(Return(shcore::Value()));
  else
    EXPECT_CALL(*this, call("dropMetadataSchema", args))
        .WillOnce(Throw(shcore::Exception::logic_error("Some error")));
}

void Mock_dba::expect_configure_local_instance(
    const std::string &uri, const nullable<std::string> &cnf_path,
    const nullable<std::string> &admin_user,
    const nullable<std::string> &admin_password,
    const nullable<bool> &clean_read_only, bool succeed) {
  shcore::Argument_list args;
  auto data = shcore::get_connection_options(uri);
  auto instance_map = mysqlsh::get_connection_map(data);
  args.push_back(shcore::Value(instance_map));

  auto options = shcore::Value::new_map();
  args.push_back(options);
  auto option_map = options.as_map();

  if (cnf_path)
    (*option_map)["mycnfPath"] = shcore::Value(*cnf_path);

  if (admin_user)
    (*option_map)["clusterAdmin"] = shcore::Value(*admin_user);

  if (admin_password)
    (*option_map)["clusterAdminPassword"] = shcore::Value(*admin_password);

  if (!clean_read_only.is_null())
    (*option_map)["clearReadOnly"] = shcore::Value(*clean_read_only);

  if (succeed) {
    auto ret_val = shcore::Value::new_map();
    auto map = ret_val.as_map();
    (*map)["status"] = shcore::Value("ok");
    EXPECT_CALL(*this, call("configureLocalInstance", args))
        .WillOnce(Return(ret_val));
  } else {
    EXPECT_CALL(*this, call("configureLocalInstance", args))
        .WillOnce(Throw(shcore::Exception::logic_error("Some error")));
  }
}

void Mock_dba::expect_create_cluster(const std::string &name,
                                     const nullable<bool> &multi_master,
                                     const nullable<bool> &force,
                                     const nullable<bool> &adopt_from_gr,
                                     const nullable<bool> &clean_read_only,
                                     bool succeed) {
  shcore::Argument_list args;
  args.push_back(shcore::Value(name));
  auto options = shcore::Value::new_map();
  auto option_map = options.as_map();

  if (!multi_master.is_null())
    (*option_map)["multiMaster"] = shcore::Value(*multi_master);

  if (!force.is_null())
    (*option_map)["force"] = shcore::Value(*force);

  if (!adopt_from_gr.is_null())
    (*option_map)["adoptFromGR"] = shcore::Value(*adopt_from_gr);

  if (!clean_read_only.is_null())
    (*option_map)["clearReadOnly"] = shcore::Value(*clean_read_only);

  args.push_back(options);

  EXPECT_CALL(*this, get_active_shell_session())
      .WillRepeatedly(Return(nullptr));

  if (succeed)
    EXPECT_CALL(*this, call("createCluster", args))
        .WillOnce(
            Return(create_mock_cluster(name)));
  else
    EXPECT_CALL(*this, call("createCluster", args))
        .WillOnce(Throw(shcore::Exception::logic_error("Some error")));
}

void Mock_dba::expect_reboot_cluster(const nullable<std::string> &name,
                                     const shcore::Value::Array_type_ref remove,
                                     const shcore::Value::Array_type_ref rejoin,
                                     const nullable<bool> &clean_read_only,
                                     bool succeed) {
  shcore::Argument_list args;

  if (name)
    args.push_back(shcore::Value(*name));

  if (remove || rejoin || !clean_read_only.is_null()) {
    auto options = shcore::Value::new_map();
    auto option_map = options.as_map();

    if (remove)
      (*option_map)["multiMaster"] = shcore::Value(remove);

    if (rejoin)
      (*option_map)["force"] = shcore::Value(rejoin);

    if (!clean_read_only.is_null())
      (*option_map)["clearReadOnly"] = shcore::Value(*clean_read_only);

    args.push_back(options);
  }

  if (succeed)
    EXPECT_CALL(*this, call("rebootClusterFromCompleteOutage", args))
        .WillOnce(Return(shcore::Value()));
  else
    EXPECT_CALL(*this, call("rebootClusterFromCompleteOutage", args))
        .WillOnce(Throw(shcore::Exception::logic_error("Some error")));
}

}  // namespace testing

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

#ifndef MODULES_ADMINAPI_DBA_UPGRADE_METADATA_H_
#define MODULES_ADMINAPI_DBA_UPGRADE_METADATA_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/command_interface.h"

namespace mysqlsh {
namespace dba {

class Upgrade_metadata : public Command_interface {
 public:
  Upgrade_metadata(const std::shared_ptr<MetadataStorage> &metadata,
                   bool interactive, bool dry_run);

  void prepare() override;
  shcore::Value execute() override;
  void rollback() override;
  void finish() override;

 private:
  void prepare_rolling_upgrade();
  shcore::Array_t get_outdated_routers();
  void upgrade_router_users();
  void print_router_list(const shcore::Array_t &routers);
  void unregister_routers(const shcore::Array_t &routers);

  const std::shared_ptr<MetadataStorage> m_metadata;
  const std::shared_ptr<Instance> m_target_instance;
  const bool m_interactive;
  const bool m_dry_run;
  bool m_abort_rolling_upgrade;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_UPGRADE_METADATA_H_

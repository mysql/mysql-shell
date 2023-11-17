/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_DBA_CONFIGURE_LOCAL_INSTANCE_H_
#define MODULES_ADMINAPI_DBA_CONFIGURE_LOCAL_INSTANCE_H_

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/dba/configure_instance.h"

namespace mysqlsh::dba {

class Configure_local_instance : private Configure_instance {
 public:
  Configure_local_instance() = delete;

  explicit Configure_local_instance(
      const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
      const Configure_instance_options &options, Cluster_type purpose);

  Configure_local_instance(const Configure_local_instance &) = delete;
  Configure_local_instance(Configure_local_instance &&) = delete;
  Configure_local_instance &operator=(const Configure_local_instance &) =
      delete;
  Configure_local_instance &operator=(Configure_local_instance &&) = delete;

  ~Configure_local_instance() = default;

 protected:
  void do_run();
  static constexpr bool supports_undo() noexcept { return false; }

 private:
  void prepare();

 private:
  TargetType::Type m_instance_type;
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_DBA_CONFIGURE_LOCAL_INSTANCE_H_

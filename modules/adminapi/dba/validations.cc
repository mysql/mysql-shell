/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/dba/validations.h"
#include "modules/adminapi/dba/check_instance.h"

namespace mysqlsh {
namespace dba {

void ensure_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance,
    std::shared_ptr<ProvisioningInterface> mp,
    std::shared_ptr<mysqlsh::IConsole> console) {
  console->println("Validating instance at " + target_instance->descr() +
                   "...");

  Check_instance check(target_instance, "", mp, console, true);
  check.prepare();
  shcore::Value result = check.execute();
  check.finish();

  if (result.as_map()->at("status").as_string() == "ok") {
    console->println("Instance configuration is suitable.");
  } else {
    console->print_error(
        "Instance must be configured and validated with "
        "dba.checkInstanceConfiguration() and dba.configureInstance() "
        "before it can be used in an InnoDB cluster.");
    throw shcore::Exception::runtime_error("Instance check failed");
  }
}

}  // namespace dba
}  // namespace mysqlsh

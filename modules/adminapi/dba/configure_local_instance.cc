/*
 * Copyright (c) 2017, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/dba/configure_local_instance.h"

#include <algorithm>
#include <string>
#include <vector>

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/mod_dba.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

Configure_local_instance::Configure_local_instance(
    const std::shared_ptr<mysqlsh::dba::Instance> &target_instance,
    const Configure_instance_options &options)
    : Configure_instance(target_instance, options, TargetType::Type::Unknown),
      m_instance_type(TargetType::Unknown) {}

Configure_local_instance::~Configure_local_instance() {}

/*
 * Validates the parameter and performs other validations regarding
 * the command execution
 */
void Configure_local_instance::prepare() {
  // Check if the target instance is local
  m_local_target = !m_target_instance->get_connection_options().has_host() ||
                   mysqlshdk::utils::Net::is_local_address(
                       m_target_instance->get_connection_options().get_host());

  // Check if we are dealing with a sandbox instance
  std::string cnf_path;
  m_is_sandbox = is_sandbox(*m_target_instance, &cnf_path);
  // if instance is sandbox and the mycnf path is empty, fill it.
  if (m_is_sandbox && m_options.mycnf_path.empty()) {
    m_options.mycnf_path = std::move(cnf_path);
    m_is_cnf_from_sandbox = true;
  }

  m_instance_type = get_gr_instance_type(*m_target_instance);

  // configure_local_instance() has 2 different operation modes:
  // - configuring instance before it joins a cluster
  // - persisting config options (like gr_group_seeds_list) after instance
  // joins a cluster
  //
  // The 1st use is replaced with configureInstance() while the 2nd should
  // be replaced with a specialized function.
  // This function should be deprecated.

  // Parameters validation for the case we only need to persist GR options
  if (m_instance_type == TargetType::InnoDBCluster) {
    auto console = mysqlsh::current_console();

    console->print_info("The instance '" + m_target_instance->descr() +
                        "' belongs to an InnoDB cluster.");

    if (m_target_instance->get_version() >=
        mysqlshdk::utils::Version(8, 0, 5)) {
      console->print_info(
          "Calling this function on a cluster member is only required for "
          "MySQL versions 8.0.4 or earlier.");
      return;
    }

    // Validate if the instance is local or not
    if (!m_local_target) {
      throw shcore::Exception::runtime_error(
          "This function only works with local instances");
    }

    if (!check_config_path_for_update()) {
      throw shcore::Exception::runtime_error(
          "Unable to update MySQL configuration file.");
    } else {
      // Set the internal configuration object properly according to the given
      // command options (to write configuration to the option file.
      prepare_config_object();
    }
  } else {
    // Do the rest of preparations same way as Configure_instance
    Configure_instance::prepare();
  }
}

/*
 * Executes the API command.
 */
shcore::Value Configure_local_instance::execute() {
  shcore::Value ret_val;
  auto console = mysqlsh::current_console();

  // Execute the configure local instance operation
  if (m_instance_type == TargetType::InnoDBCluster) {
    if (m_target_instance->get_version() >= mysqlshdk::utils::Version(8, 0, 5))
      return {};
    console->print_info("Persisting the cluster settings...");

    // make sure super_read_only=1 is persisted to disk
    m_cfg->set_for_handler("super_read_only",
                           mysqlshdk::utils::nullable<bool>(true),
                           mysqlshdk::config::k_dft_cfg_file_handler);

    mysqlsh::dba::persist_gr_configurations(*m_target_instance, m_cfg.get());

    console->print_info("The instance '" + m_target_instance->descr() +
                        "' was configured for use in an InnoDB cluster.");

    console->print_info();
    console->print_info(
        "The instance cluster settings were successfully persisted.");

    return shcore::Value();
  } else {
    return Configure_instance::execute();
  }
}
}  // namespace dba
}  // namespace mysqlsh

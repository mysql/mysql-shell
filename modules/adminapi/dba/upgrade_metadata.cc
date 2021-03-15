/*
 * Copyright (c) 2019, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/dba/upgrade_metadata.h"

#include <algorithm>
#include <string>
#include <vector>

#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/metadata_management_mysql.h"
#include "modules/adminapi/common/router.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/include/shellcore/shell_resultset_dumper.h"
#include "mysqlshdk/libs/mysql/utils.h"
#include "mysqlshdk/libs/utils/array_result.h"
#include "mysqlshdk/libs/utils/compiler.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {

using MDState = mysqlsh::dba::metadata::State;

Upgrade_metadata::Upgrade_metadata(
    const std::shared_ptr<MetadataStorage> &metadata, bool interactive,
    bool dry_run)
    : m_metadata(metadata),
      m_target_instance(m_metadata->get_md_server()),
      m_interactive(interactive),
      m_dry_run(dry_run),
      m_abort_rolling_upgrade(false) {}

/*
 * Validates the parameter and performs other validations regarding
 * the command execution
 */
void Upgrade_metadata::prepare() {
  // Acquire required locks on target instance.
  // No "write" operation allowed to be executed concurrently on the target
  // instance.
  m_target_instance->get_lock_exclusive();

  std::string current_user, current_host;
  m_target_instance->get_current_user(&current_user, &current_host);

  std::string error_info;
  auto console = mysqlsh::current_console();
  if (!validate_cluster_admin_user_privileges(*m_target_instance, current_user,
                                              current_host, &error_info)) {
    console->print_error(error_info);
    throw shcore::Exception::runtime_error(
        "The account " + shcore::make_account(current_user, current_host) +
        " is missing privileges required for this operation.");
  }

  mysqlshdk::utils::Version installed,
      current = mysqlsh::dba::metadata::current_version();
  m_metadata->check_version(&installed);

  auto instance_data = m_target_instance->descr();

  // If the installed version is lower than the current one, we first validate
  // if it is valid before continuing
  if (installed < current && !metadata::is_valid_version(installed)) {
    throw shcore::Exception::runtime_error(
        "Installed metadata at '" + instance_data +
        "' has an unknown version (" + installed.get_base() +
        "), upgrading this version of the metadata is not supported.");
  }

  switch (m_metadata->get_state()) {
    case MDState::EQUAL:
      console->print_note("Installed metadata at '" + instance_data +
                          "' is up to date (version " + installed.get_base() +
                          ").");
      break;
    case MDState::MAJOR_HIGHER:
    case MDState::MINOR_HIGHER:
    case MDState::PATCH_HIGHER:
      throw std::runtime_error(
          "Installed metadata at '" + instance_data +
          "' is newer than the version version supported by "
          "this Shell (installed: " +
          installed.get_base() + ", shell: " + current.get_base() + ").");

      break;
    case MDState::PATCH_LOWER:
    case MDState::MINOR_LOWER:
    case MDState::MAJOR_LOWER:
      console->print_info(shcore::str_format(
          "InnoDB Cluster Metadata Upgrade\n\n"
          "The cluster you are connected to is using an outdated metadata "
          "schema version %s and needs to be upgraded to %s.\n\n"
          "Without doing this upgrade, no AdminAPI calls except read only "
          "operations will be allowed.",
          installed.get_base().c_str(), current.get_base().c_str()));

      if (m_metadata->get_state() == MDState::MAJOR_LOWER) {
        console->print_info();
        console->print_note(
            "After the upgrade, this InnoDB Cluster/ReplicaSet can no longer "
            "be managed using older versions of MySQL Shell.");
      }

      if (m_metadata->get_state() != MDState::PATCH_LOWER)
        prepare_rolling_upgrade();
      break;
    case MDState::UPGRADING:
      throw std::runtime_error(shcore::str_subvars(
          mysqlsh::dba::metadata::kFailedUpgradeError,
          [](const std::string &var) {
            return shcore::get_member_name(var, shcore::current_naming_style());
          },
          "<<<", ">>>"));
      break;
    case MDState::FAILED_SETUP:
      throw std::runtime_error(
          "Metadata setup is not finished. Incomplete metadata schema must be"
          " dropped.");

    case MDState::FAILED_UPGRADE:
      // Nothing to do, prepare succeeds and lets the logic go right to
      // execute()
      break;
    case MDState::NONEXISTING:
      // NO OP: Preconditions won't allow this case to be called
      break;
  }
}

shcore::Array_t Upgrade_metadata::get_outdated_routers() {
  auto routers_dict =
      mysqlsh::dba::router_list(m_metadata.get(), "", true).as_map();

  shcore::Array_t routers;

  if (!routers_dict->empty()) {
    routers = shcore::make_array();

    auto columns = shcore::make_array();
    columns->emplace_back("Instance");
    columns->emplace_back("Version");
    columns->emplace_back("Last Check-in");
    columns->emplace_back("R/O Port");
    columns->emplace_back("R/W Port");

    routers->emplace_back(std::move(columns));

    for (const auto &router_md : *routers_dict) {
      auto router = shcore::make_array();
      router->emplace_back(router_md.first);
      auto router_data = router_md.second.as_map();
      router->push_back((*router_data)["version"]);
      router->push_back((*router_data)["lastCheckin"]);
      router->push_back((*router_data)["roPort"]);
      router->push_back((*router_data)["rwPort"]);

      routers->emplace_back(std::move(router));
    }
  }

  return routers;
}

void Upgrade_metadata::prepare_rolling_upgrade() {
  // The router accounts need to be upgraded first, no matter if there are
  // outdated routers or not, i.e. router 8.0.19 could have been used to
  // bootstrap under MD 1.0.1 in which case the account will be incomplete but
  // the router will be up to date
  upgrade_router_users();

  auto routers = get_outdated_routers();

  if (routers) {
    auto console = current_console();

    // For patch upgrades, rolling upgrade is not considered
    bool done_router_upgrade = m_metadata->get_state() == MDState::PATCH_LOWER;

    // Doing the rolling upgrade is mandatory for a MAJOR upgrade, on a minor
    // upgrade the user can decide whether to do it or not
    bool do_rolling_upgrade = m_metadata->get_state() == MDState::MAJOR_LOWER;

    if (!done_router_upgrade) {
      console->println(shcore::str_format(
          "An upgrade of all cluster router instances is %s. All router "
          "installations should be updated first before doing the actual "
          "metadata upgrade.\n",
          do_rolling_upgrade ? "required" : "recommended"));

      print_router_list(routers);

      if (m_interactive && !m_dry_run) {
        if (!do_rolling_upgrade) {
          if (console->confirm(
                  "Do you want to proceed with the upgrade (Yes/No)") ==
              mysqlsh::Prompt_answer::YES) {
            do_rolling_upgrade = true;
          } else {
            // If user was given the option to do rolling upgrade and he chooses
            // not to do it, it's ok, we should continue
            done_router_upgrade = true;
          }
        }

        while (do_rolling_upgrade && routers) {
          size_t count = routers->size() - 1;

          std::string answer;

          console->println(shcore::str_format(
              "There %s %zu Router%s to upgrade. Please "
              "upgrade %s and select Continue once %s restarted.\n",
              (count == 1) ? "is" : "are", count, (count == 1) ? "" : "s",
              (count == 1) ? "it" : "them",
              (count == 1) ? "it is" : "they are"));

          std::vector<std::string> options = {
              "Re-check for outdated Routers and continue with the metadata "
              "upgrade.",
              "Unregister the remaining Routers.", "Abort the operation.",
              "Help"};

          if (console->select(
                  "Please select an option: ", &answer, options, 0UL, true,
                  [&options](const std::string &a) -> std::string {
                    if (a.size() == 1) {
                      if (a.find_first_of("?aArCuUhH") == std::string::npos) {
                        return "Invalid option selected";
                      }
                    } else {
                      if (std::find(options.begin(), options.end(), a) ==
                              std::end(options) &&
                          !shcore::str_caseeq(a, "abort") &&
                          !shcore::str_caseeq(a, "continue") &&
                          !shcore::str_caseeq(a, "unregister") &&
                          !shcore::str_caseeq(a, "help")) {
                        return "Invalid option selected";
                      }
                    }
                    return "";
                  })) {
            switch (answer[0]) {
              case 'U':
              case 'u':
                if (console->confirm(
                        "Unregistering a Router implies it will not "
                        "be used in the "
                        "Cluster, do you want to continue?") ==
                    mysqlsh::Prompt_answer::YES) {
                  // First row is the table headers
                  routers->erase(routers->begin());
                  unregister_routers(routers);
                }
                // NOTE: This fallback is required, after unregistering the
                // listed routers it will refresh the list if needed (implicit
                // retry)
                FALLTHROUGH;
              case 'R':
              case 'r':
                DBUG_EXECUTE_IF("dba_EMULATE_ROUTER_UNREGISTER", {
                  m_target_instance->execute(
                      "DELETE FROM mysql_innodb_cluster_metadata.routers WHERE "
                      "router_id = 2");
                });
                DBUG_EXECUTE_IF("dba_EMULATE_ROUTER_UPGRADE", {
                  m_target_instance->execute(
                      "UPDATE mysql_innodb_cluster_metadata.routers SET "
                      "attributes=JSON_OBJECT('version','8.0.19') WHERE "
                      "router_id = 2");
                });

                routers = get_outdated_routers();
                if (routers) {
                  print_router_list(routers);
                } else {
                  done_router_upgrade = true;
                }
                break;
              case 'A':
              case 'a':
                do_rolling_upgrade = false;
                m_abort_rolling_upgrade = true;
                break;
              case 'H':
              case 'h':
              case '?':
                console->println(shcore::str_subvars(
                    "To perform a rolling upgrade of the InnoDB "
                    "Cluster/ReplicaSet metadata, execute the following "
                    "steps:\n\n"
                    "1 - Upgrade the Shell to the latest version\n"
                    "2 - Execute dba.<<<upgradeMetadata>>>() (the current "
                    "step)\n"
                    "3 - Upgrade MySQL Router instances to the latest "
                    "version\n"
                    "4 - Continue with the metadata upgrade once all Router "
                    "instances are upgraded or accounted for\n",
                    [](const std::string &var) {
                      return shcore::get_member_name(
                          var, shcore::current_naming_style());
                    },
                    "<<<", ">>>"));

                routers = get_outdated_routers();
                if (routers) {
                  console->println(
                      "If the following Router instances no longer exist, "
                      "select Unregister to delete their metadata.");
                  print_router_list(routers);
                }
                break;
            }
          } else {
            // Ctrl+C was hit
            do_rolling_upgrade = false;
            m_abort_rolling_upgrade = true;
          }
        }
      }

      if (!done_router_upgrade) {
        if (m_dry_run) {
          size_t count = routers->size() - 1;
          console->println(shcore::str_format(
              "There %s %zu Router%s to be upgraded in order to perform the "
              "Metadata schema upgrade.\n",
              (count == 1) ? "is" : "are", count, (count == 1) ? "" : "s"));
        } else if (m_abort_rolling_upgrade) {
          console->println("The metadata upgrade has been aborted.");
        } else {
          throw shcore::Exception::runtime_error(
              "Outdated Routers found. Please upgrade the Routers before "
              "upgrading the Metadata schema");
        }
      }
    }
  }
}

void Upgrade_metadata::print_router_list(const shcore::Array_t &routers) {
  // Prints up to 10 rows, if more than 10 routers the last wor will have
  // ellipsis on each column
  const shcore::Array_t print_list = shcore::make_array();
  size_t index = 0;
  size_t count = routers->size();
  size_t max = count > 11 ? 10 : count;
  while (index < max) {
    print_list->push_back((*routers)[index]);
    index++;
  }

  if (count > 11) {
    auto etc = shcore::make_array();
    for (size_t idx = 0; idx < 5; idx++) {
      etc->push_back(shcore::Value("..."));
    }
    print_list->push_back(shcore::Value(etc));
  }

  shcore::Array_as_result result(print_list);

  mysqlsh::Resultset_dumper dumper(
      &result, mysqlsh::current_shell_options()->get().wrap_json, "table",
      false, false);
  dumper.dump("", true, false);
}

void Upgrade_metadata::unregister_routers(const shcore::Array_t &routers) {
  for (const auto &router : *routers) {
    auto router_def = router.as_array();
    m_metadata->remove_router((*router_def)[0].as_string());
  }
}

void Upgrade_metadata::upgrade_router_users() {
  std::vector<std::pair<std::string, std::string>> users;
  auto result = m_target_instance->query(
      "SELECT user, host FROM mysql.user WHERE user LIKE "
      "'mysql_router%'");

  auto row = result->fetch_one();
  while (row) {
    users.push_back({row->get_string(0), row->get_string(1)});
    row = result->fetch_one();
  }

  size_t size = users.size();

  auto console = current_console();

  console->print_info(
      "\nThe grants for the MySQL Router accounts that were created "
      "automatically when bootstrapping need to be updated to match the new "
      "metadata version's requirements.");

  if (!users.empty()) {
    console->print_info(
        m_dry_run
            ? shcore::str_format("%zi Router account%s need to be updated.",
                                 size, (size == 1) ? "" : "s")
            : "Updating Router accounts...");

    if (!m_dry_run) {
      std::vector<std::string> queries = {
          "GRANT SELECT, EXECUTE ON mysql_innodb_cluster_metadata.* TO ",
          "GRANT SELECT ON performance_schema.replication_group_members TO ",
          "GRANT SELECT ON performance_schema.replication_group_member_stats "
          "TO ",
          "GRANT SELECT ON performance_schema.global_variables TO ",
          "GRANT UPDATE, INSERT, DELETE ON "
          "mysql_innodb_cluster_metadata.v2_routers TO ",
          "GRANT UPDATE, INSERT, DELETE ON "
          "mysql_innodb_cluster_metadata.routers TO "};

      mysqlshdk::utils::Version installed;
      m_metadata->check_version(&installed);

      // When migrating to version 2.0.0 this dummy view is required to
      // enable giving the router users the  right permissions, it will be
      // updated to the correct view once the upgrade is done
      if (installed < mysqlshdk::utils::Version(2, 0, 0)) {
        m_target_instance->execute(
            "CREATE OR REPLACE VIEW mysql_innodb_cluster_metadata.v2_routers "
            "AS SELECT 1");
      }

      log_debug("Metadata upgrade for version %s, upgraded Router accounts:",
                mysqlsh::dba::metadata::current_version().get_base().c_str());
      for (const auto &user : users) {
        shcore::sqlstring user_host("!@!", 0);
        user_host << std::get<0>(user);
        user_host << std::get<1>(user);

        std::string str_user = user_host.str();

        log_debug("- %s", str_user.c_str());

        for (const auto &query : queries) {
          m_target_instance->execute(query + str_user);
        }
      }
      console->print_note(shcore::str_format(
          "%zi Router account%s %s been updated.", size, (size == 1) ? "" : "s",
          (size == 1 ? "has" : "have")));
    }
  } else {
    console->print_note("No automatically created Router accounts were found.");
  }

  console->print_warning(
      "If MySQL Routers have been bootstrapped using custom accounts, their "
      "grants can not be updated during the metadata upgrade, they have to "
      "be updated using the <<<setupRouterAccount>>> function. \nFor "
      "additional information use: \\? <<<setupRouterAccount>>>\n");
}

/*
 * Executes the API command.
 */
shcore::Value Upgrade_metadata::execute() {
  if (!m_abort_rolling_upgrade) {
    metadata::upgrade_or_restore_schema(m_target_instance, m_dry_run);
  }

  return shcore::Value();
}

void Upgrade_metadata::rollback() {}

void Upgrade_metadata::finish() {
  if (m_target_instance) {
    // Release locks at the end.
    m_target_instance->release_lock();
  }
}

}  // namespace dba
}  // namespace mysqlsh

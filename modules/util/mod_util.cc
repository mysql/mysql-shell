/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/util/mod_util.h"
#include <memory>
#include <vector>
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/util/upgrade_check.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shellcore/utils_help.h"

namespace mysqlsh {

REGISTER_HELP(
    UTIL_INTERACTIVE_BRIEF,
    "Global object that groups miscellaneous tools like upgrade checker.");
REGISTER_HELP(
    UTIL_BRIEF,
    "Global object that groups miscellaneous tools like upgrade checker.");

Util::Util(shcore::IShell_core* owner) : _shell_core(*owner) {
  add_method("checkForServerUpgrade", std::bind(&Util::check_for_server_upgrade,
                                                this, std::placeholders::_1),
             "data", shcore::Map, NULL);
}

static std::string format_upgrade_issue(const Upgrade_issue& problem) {
  std::stringstream ss;
  const char* item = "Schema";
  ss << problem.schema;
  if (!problem.table.empty()) {
    item = "Table";
    ss << "." << problem.table;
    if (!problem.column.empty()) {
      item = "Column";
      ss << "." << problem.column;
    }
  }

  return shcore::str_format(
      "%-8s %s (%s) - %s",
      Upgrade_issue::level_to_string(problem.level).c_str(), item,
      ss.str().c_str(), problem.description.c_str());
}

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_BRIEF,
              "Performs series of tests on specified MySQL server to check if "
              "the upgrade process will succeed.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_PARAM,
              "@param connectionData Optional the connection data to server to "
              "be checked");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_PARAM1,
              "@param password Optional password for the session");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_RETURNS,
              "@returns 0 when no problems were found, 1 when no fatal errors "
              "were found and 2 when errors blocking upgrade process were "
              "discovered.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL,
              "If no connectionData is specified tool will try to establish "
              "connection using data from current session.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL1, "TOPIC_CONNECTION_DATA");

/**
 * \ingroup util
 * $(UTIL_CHECKFORSERVERUPGRADE_BRIEF)
 *
 * $(UTIL_CHECKFORSERVERUPGRADE_PARAM)
 * $(UTIL_CHECKFORSERVERUPGRADE_PARAM1)
 *
 * $(UTIL_CHECKFORSERVERUPGRADE_RETURNS)
 *
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref
 * connection_data
 *
 */
#if DOXYGEN_JS
Integer Util::checkForServerUpgrade(ConnectionData connectionData,
                                    String password);
#elif DOXYGEN_PY
int Util::check_for_server_upgrade(ConnectionData connectionData, str password);
#endif

shcore::Value Util::check_for_server_upgrade(
    const shcore::Argument_list& args) {
  args.ensure_count(0, 2, get_function_name("checkForServerUpgrade").c_str());
  int errors = 0, warnings = 0, notices = 0;
  try {
    if (args.size() == 0 && !_shell_core.get_dev_session())
      throw shcore::Exception::argument_error(
          "Please connect the shell to the MySQL server to be checked or "
          "specify the server URI as a parameter.");

    Connection_options connection_options =
        args.size() == 0
            ? _shell_core.get_dev_session()->get_connection_options()
            : mysqlsh::get_connection_options(args,
                                              mysqlsh::PasswordFormat::STRING);

    _shell_core.print(shcore::str_format(
        "The MySQL server at %s will now be checked for compatibility "
        "issues for upgrade to MySQL 8.0...\n",
        connection_options.as_uri(mysqlshdk::db::uri::formats::only_transport())
            .c_str()));

    std::shared_ptr<mysqlshdk::db::ISession> session =
        mysqlshdk::db::mysql::Session::create();
    session->connect(connection_options);

    auto result = session->query("select current_user();");
    const mysqlshdk::db::IRow* row = result->fetch_one();
    if (row == nullptr)
      throw std::runtime_error("Unable to get information about a user");

    std::string user;
    std::string host;
    shcore::split_account(row->get_string(0), &user, &host);

    mysqlshdk::mysql::Instance instance(session);
    auto res = instance.check_user(user, host, {"all"}, "*", "*");
    if ("" != std::get<1>(res))
      throw std::invalid_argument(
          "The upgrade check needs to be performed by user with ALL "
          "privileges.");

    auto version_result = session->query("select @@version, @@version_comment");
    row = version_result->fetch_one();
    if (row == nullptr)
      throw std::runtime_error("Unable to get server version");
    std::string current_version = row->get_string(0);
    _shell_core.print(shcore::str_format("MySQL version: %s - %s\n",
                                         current_version.c_str(),
                                         row->get_string(1).c_str()));

    auto checklist = Upgrade_check::create_checklist(current_version, "8.0");

    for (size_t i = 0; i < checklist.size(); i++) {
      _shell_core.print(
          shcore::str_format("\n%zu) %s\n", i + 1, checklist[i]->get_name()));
      std::vector<Upgrade_issue> issues = checklist[i]->run(session);

      std::function<std::string(const Upgrade_issue&)> issue_formater(
          to_string);
      if (issues.empty())
        _shell_core.print("  No issues found\n");
      else if (checklist[i]->get_long_advice() != nullptr)
        _shell_core.print(
            shcore::str_format("  %s\n\n", checklist[i]->get_long_advice()));
      else
        issue_formater = format_upgrade_issue;

      for (const auto& issue : issues) {
        switch (issue.level) {
          case Upgrade_issue::ERROR:
            errors++;
            break;
          case Upgrade_issue::WARNING:
            warnings++;
            break;
          default:
            notices++;
            break;
        }
        _shell_core.print(
            shcore::str_format("  %s\n", issue_formater(issue).c_str()));
      }
    }
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("checkForServerUpgrade"));

  _shell_core.print("\n");
  if (errors > 0) {
    _shell_core.print(shcore::str_format(
        "%i errors were found. Please correct these issues before upgrading to "
        "MySQL 8 to avoid compatibility issues.\n",
        errors));
    return shcore::Value(2);
  }
  if (warnings > 0) {
    _shell_core.print(
        "No fatal errors were found that would prevent a MySQL 8 upgrade, but "
        "some potential issues were detected. Please ensure that the reported "
        "issues are not significant before upgrading.\n");
    return shcore::Value(1);
  }
  if (notices > 0)
    _shell_core.print(
        "No fatal errors were found that would prevent a MySQL 8 upgrade, but "
        "some potential issues were detected. Please ensure that the reported "
        "issues are not significant before upgrading.\n");
  else
    _shell_core.print(
        "No known compatibility errors or issues for upgrading the target "
        "server to MySQL 8 were found.\n");
  return shcore::Value(0);
}

} /* namespace mysqlsh */

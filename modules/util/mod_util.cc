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
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "shellcore/utils_help.h"

namespace mysqlsh {

REGISTER_HELP_OBJECT(util, shellapi);
REGISTER_HELP(
    UTIL_GLOBAL_BRIEF,
    "Global object that groups miscellaneous tools like upgrade checker.");
REGISTER_HELP(
    UTIL_BRIEF,
    "Global object that groups miscellaneous tools like upgrade checker.");

Util::Util(shcore::IShell_core *owner,
           std::shared_ptr<mysqlsh::IConsole> console_handler)
    : _shell_core(*owner), m_console_handler(console_handler) {
  add_method(
      "checkForServerUpgrade",
      std::bind(&Util::check_for_server_upgrade, this, std::placeholders::_1),
      "data", shcore::Map);
}

static std::string format_upgrade_issue(const Upgrade_issue &problem) {
  std::stringstream ss;
  const char *item = "Schema";
  ss << problem.schema;
  if (!problem.table.empty()) {
    item = "Table";
    ss << "." << problem.table;
    if (!problem.column.empty()) {
      item = "Column";
      ss << "." << problem.column;
    }
  }

  return shcore::str_format("%-8s: %s (%s) - %s",
                            Upgrade_issue::level_to_string(problem.level), item,
                            ss.str().c_str(), problem.description.c_str());
}

REGISTER_HELP_FUNCTION(checkForServerUpgrade, util);
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_BRIEF,
              "Performs series of tests on specified MySQL server to check if "
              "the upgrade process will succeed.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_PARAM,
              "@param connectionData Optional the connection data to server to "
              "be checked");
REGISTER_HELP(
    UTIL_CHECKFORSERVERUPGRADE_PARAM1,
    "@param options Optional dictionary of options to modify tool behaviour.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_RETURNS,
              "@returns 0 when no problems were found, 1 when no fatal errors "
              "were found and 2 when errors blocking upgrade process were "
              "discovered.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL,
              "If no connectionData is specified tool will try to establish "
              "connection using data from current session.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL1,
              "Tool behaviour can be modified with following options:");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL2,
              "@li outputFormat - value can be either TEXT (default) or JSON.");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL3,
              "@li password - password for connection.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL4, "${TOPIC_CONNECTION_DATA}");

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
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL1)
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL2)
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL3)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref
 * connection_data
 *
 */
#if DOXYGEN_JS
Integer Util::checkForServerUpgrade(ConnectionData connectionData,
                                    Dictionary options);
#elif DOXYGEN_PY
int Util::check_for_server_upgrade(ConnectionData connectionData, dict options);
#endif

class Upgrade_check_output_formatter {
 public:
  static std::unique_ptr<Upgrade_check_output_formatter> get_formatter(
      shcore::IShell_core *core, const std::string &format);

  virtual ~Upgrade_check_output_formatter() {}

  virtual void check_info(const std::string &server_addres,
                          const std::string &server_version,
                          const std::string &target_version) = 0;
  virtual void check_results(const Upgrade_check &check,
                             const std::vector<Upgrade_issue> &results) = 0;
  virtual void check_error(const Upgrade_check &check,
                           const char *description) = 0;
  virtual void summarize(int error, int warning, int notice,
                         const std::string &text) = 0;

 protected:
  explicit Upgrade_check_output_formatter(shcore::IShell_core *core)
      : m_shell_core(*core) {}

  shcore::IShell_core &m_shell_core;
};

class Text_upgrade_checker_output : public Upgrade_check_output_formatter {
 public:
  explicit Text_upgrade_checker_output(shcore::IShell_core *core)
      : Upgrade_check_output_formatter(core) {}

  void check_info(const std::string &server_addres,
                  const std::string &server_version,
                  const std::string &target_version) override {
    m_shell_core.print(shcore::str_format(
        "The MySQL server at %s will now be checked for compatibility "
        "issues for upgrade to MySQL %s...\n",
        server_addres.c_str(), target_version.c_str()));

    m_shell_core.print(
        shcore::str_format("MySQL version: %s\n", server_version.c_str()));
  }

  void check_results(const Upgrade_check &check,
                     const std::vector<Upgrade_issue> &results) override {
    m_shell_core.print(
        shcore::str_format("\n%d) %s\n", ++check_count, check.get_title()));

    std::function<std::string(const Upgrade_issue &)> issue_formater(to_string);

    if (results.empty())
      m_shell_core.print("  No issues found\n");
    else if (check.get_description() != nullptr)
      m_shell_core.print(
          shcore::str_format("  %s\n\n", check.get_description()));
    else
      issue_formater = format_upgrade_issue;

    for (const auto &issue : results)
      m_shell_core.print(
          shcore::str_format("  %s\n", issue_formater(issue).c_str()));
  }

  void check_error(const Upgrade_check &check,
                   const char *description) override {
    m_shell_core.print(
        shcore::str_format("\n%d) %s\n", ++check_count, check.get_title()));
    m_shell_core.print_error("Check failed: ");
    m_shell_core.print_error(description);
    m_shell_core.print("\n");
  }

  void summarize(int error, int warning, int notice,
                 const std::string &text) override {
    m_shell_core.print("\n");
    m_shell_core.print(text);
  }

 private:
  int check_count = 0;
};

class JSON_upgrade_checker_output : public Upgrade_check_output_formatter {
 public:
  explicit JSON_upgrade_checker_output(shcore::IShell_core *core)
      : Upgrade_check_output_formatter(core),
        m_json_document(),
        m_allocator(m_json_document.GetAllocator()),
        m_checks(rapidjson::kArrayType) {
    m_json_document.SetObject();
  }

  void check_info(const std::string &server_addres,
                  const std::string &server_version,
                  const std::string &target_version) override {
    rapidjson::Value addr;
    addr.SetString(server_addres.c_str(), server_addres.length(), m_allocator);
    m_json_document.AddMember("serverAddress", addr, m_allocator);

    rapidjson::Value svr;
    svr.SetString(server_version.c_str(), server_version.length(), m_allocator);
    m_json_document.AddMember("serverVersion", svr, m_allocator);

    rapidjson::Value tvr;
    tvr.SetString(target_version.c_str(), target_version.length(), m_allocator);
    m_json_document.AddMember("targetVersion", tvr, m_allocator);
  }

  void check_results(const Upgrade_check &check,
                     const std::vector<Upgrade_issue> &results) override {
    rapidjson::Value check_object(rapidjson::kObjectType);
    rapidjson::Value id;
    check_object.AddMember("id", rapidjson::StringRef(check.get_name()),
                           m_allocator);
    check_object.AddMember("title", rapidjson::StringRef(check.get_title()),
                           m_allocator);
    check_object.AddMember("status", rapidjson::StringRef("OK"), m_allocator);
    if (check.get_description() != nullptr)
      check_object.AddMember("description",
                             rapidjson::StringRef(check.get_description()),
                             m_allocator);
    if (check.get_doc_link() != nullptr)
      check_object.AddMember("documentationLink",
                             rapidjson::StringRef(check.get_doc_link()),
                             m_allocator);

    rapidjson::Value issues(rapidjson::kArrayType);
    for (const auto &issue : results) {
      if (issue.empty()) continue;
      rapidjson::Value issue_object(rapidjson::kObjectType);
      issue_object.AddMember(
          "level",
          rapidjson::StringRef(Upgrade_issue::level_to_string(issue.level)),
          m_allocator);

      std::string db_object = issue.get_db_object();
      rapidjson::Value dbov;
      dbov.SetString(db_object.c_str(), db_object.length(), m_allocator);
      issue_object.AddMember("dbObject", dbov, m_allocator);

      rapidjson::Value description;
      description.SetString(issue.description.c_str(),
                            issue.description.length(), m_allocator);
      issue_object.AddMember("description", description, m_allocator);

      issues.PushBack(issue_object, m_allocator);
    }

    check_object.AddMember("detectedProblems", issues, m_allocator);
    m_checks.PushBack(check_object, m_allocator);
  }

  void check_error(const Upgrade_check &check,
                   const char *description) override {
    rapidjson::Value check_object(rapidjson::kObjectType);
    rapidjson::Value id;
    check_object.AddMember("id", rapidjson::StringRef(check.get_name()),
                           m_allocator);
    check_object.AddMember("title", rapidjson::StringRef(check.get_title()),
                           m_allocator);
    check_object.AddMember("status", rapidjson::StringRef("ERROR"),
                           m_allocator);

    rapidjson::Value descr;
    descr.SetString(description, strlen(description), m_allocator);
    check_object.AddMember("description", descr, m_allocator);
    m_checks.PushBack(check_object, m_allocator);
  }

  void summarize(int error, int warning, int notice,
                 const std::string &text) override {
    m_json_document.AddMember("errorCount", error, m_allocator);
    m_json_document.AddMember("warningCount", warning, m_allocator);
    m_json_document.AddMember("noticeCount", notice, m_allocator);

    rapidjson::Value val;
    val.SetString(text.c_str(), text.length(), m_allocator);
    m_json_document.AddMember("summary", val, m_allocator);

    m_json_document.AddMember("checksPerformed", m_checks, m_allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    m_json_document.Accept(writer);
    m_shell_core.print(buffer.GetString());
  }

 private:
  rapidjson::Document m_json_document;
  rapidjson::Document::AllocatorType &m_allocator;
  rapidjson::Value m_checks;
};

std::unique_ptr<Upgrade_check_output_formatter>
Upgrade_check_output_formatter::get_formatter(shcore::IShell_core *core,
                                              const std::string &format) {
  if (shcore::str_casecmp(format, "JSON") == 0)
    return std::unique_ptr<Upgrade_check_output_formatter>(
        new JSON_upgrade_checker_output(core));
  else if (shcore::str_casecmp(format, "TEXT") != 0)
    throw std::invalid_argument(
        "Allowed values for outputFormat parameter are TEXT or JSON");

  return std::unique_ptr<Upgrade_check_output_formatter>(
      new Text_upgrade_checker_output(core));
}

shcore::Value Util::check_for_server_upgrade(
    const shcore::Argument_list &args) {
  args.ensure_count(0, 2, get_function_name("checkForServerUpgrade").c_str());
  int errors = 0, warnings = 0, notices = 0;
  shcore::Value ret(0);

  try {
    Connection_options connection_options;
    if (args.size() != 0) {
      try {
        connection_options = mysqlsh::get_connection_options(
            args, mysqlsh::PasswordFormat::OPTIONS);
      } catch (const std::exception &e) {
        if (_shell_core.get_dev_session())
          connection_options =
              _shell_core.get_dev_session()->get_connection_options();
        else
          throw;
      }
    } else {
      if (!_shell_core.get_dev_session())
        throw shcore::Exception::argument_error(
            "Please connect the shell to the MySQL server to be checked or "
            "specify the server URI as a parameter.");
      connection_options =
          _shell_core.get_dev_session()->get_connection_options();
    }

    std::string output_format("TEXT");
    if (args.size() > 0 &&
        args[args.size() - 1].type == shcore::Value_type::Map) {
      auto dict = args.map_at(args.size() - 1);
      output_format = dict->get_string("outputFormat", "TEXT");
    }

    auto print = Upgrade_check_output_formatter::get_formatter(&_shell_core,
                                                               output_format);

    auto session = establish_session(
        connection_options, current_shell_options()->get().wizards);

    auto result = session->query("select current_user();");
    const mysqlshdk::db::IRow *row = result->fetch_one();
    if (row == nullptr)
      throw std::runtime_error("Unable to get information about a user");

    std::string user;
    std::string host;
    shcore::split_account(row->get_string(0), &user, &host);

    mysqlshdk::mysql::Instance instance(session);
    auto res = instance.get_user_privileges(user, host)->validate({"all"});
    if (res.has_missing_privileges())
      throw std::invalid_argument(
          "The upgrade check needs to be performed by user with ALL "
          "privileges.");

    auto version_result = session->query("select @@version, @@version_comment");
    row = version_result->fetch_one();
    if (row == nullptr)
      throw std::runtime_error("Unable to get server version");

    std::string current_version = row->get_string(0);

    print->check_info(connection_options.as_uri(
                          mysqlshdk::db::uri::formats::only_transport()),
                      current_version + " - " + row->get_string(1), "8.0");

    auto checklist = Upgrade_check::create_checklist(current_version, "8.0.11");

    for (size_t i = 0; i < checklist.size(); i++) try {
        std::vector<Upgrade_issue> issues = checklist[i]->run(session);

        for (const auto &issue : issues) switch (issue.level) {
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

        print->check_results(*checklist[i], issues);
      } catch (const std::exception &e) {
        print->check_error(*checklist[i], e.what());
      }

    std::string summary;
    if (errors > 0) {
      summary = shcore::str_format(
          "%i errors were found. Please correct these issues before upgrading "
          "to MySQL 8 to avoid compatibility issues.\n",
          errors);
      ret = shcore::Value(2);
    } else if (warnings > 0) {
      summary =
          "No fatal errors were found that would prevent a MySQL 8 upgrade, "
          "but some potential issues were detected. Please ensure that the "
          "reported issues are not significant before upgrading.\n";
      ret = shcore::Value(1);
    } else if (notices > 0) {
      summary =
          "No fatal errors were found that would prevent a MySQL 8 upgrade, "
          "but some potential issues were detected. Please ensure that the "
          "reported issues are not significant before upgrading.\n";
    } else {
      summary =
          "No known compatibility errors or issues for upgrading the target "
          "server to MySQL 8 were found.\n";
    }
    print->summarize(errors, warnings, notices, summary);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("checkForServerUpgrade"));

  return ret;
}

} /* namespace mysqlsh */

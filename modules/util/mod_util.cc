/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include <set>
#include <vector>
#include "modules/mod_utils.h"
#include "modules/mysqlxtest_utils.h"
#include "modules/util/import_table/import_table.h"
#include "modules/util/import_table/import_table_options.h"
#include "modules/util/json_importer.h"
#include "modules/util/upgrade_check.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/document_parser.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "shellcore/interrupt_handler.h"
#include "shellcore/utils_help.h"

#ifdef WITH_OCI
#include "modules/util/oci_setup.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#endif

namespace mysqlsh {

REGISTER_HELP_GLOBAL_OBJECT(util, shellapi);
REGISTER_HELP(UTIL_GLOBAL_BRIEF,
              "Global object that groups miscellaneous tools like upgrade "
              "checker and JSON import.");
REGISTER_HELP(UTIL_BRIEF,
              "Global object that groups miscellaneous tools like upgrade "
              "checker and JSON import.");

Util::Util(shcore::IShell_core *owner) : _shell_core(*owner) {
  add_method(
      "checkForServerUpgrade",
      std::bind(&Util::check_for_server_upgrade, this, std::placeholders::_1),
      "data", shcore::Map);

  expose("importJson", &Util::import_json, "path", "?options");
#ifdef WITH_OCI
  shcore::ssl::init();
  expose("configureOci", &Util::configure_oci, "?profile");
#endif
  expose("importTable", &Util::import_table, "path", "?options");
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

class Upgrade_check_output_formatter {
 public:
  static std::unique_ptr<Upgrade_check_output_formatter> get_formatter(
      const std::string &format);

  virtual ~Upgrade_check_output_formatter() {}

  virtual void check_info(const std::string &server_addres,
                          const std::string &server_version,
                          const std::string &target_version) = 0;
  virtual void check_results(const Upgrade_check &check,
                             const std::vector<Upgrade_issue> &results) = 0;
  virtual void check_error(const Upgrade_check &check, const char *description,
                           bool runtime_error = true) = 0;
  virtual void manual_check(const Upgrade_check &check) = 0;
  virtual void summarize(int error, int warning, int notice,
                         const std::string &text) = 0;
};

class Text_upgrade_checker_output : public Upgrade_check_output_formatter {
 public:
  Text_upgrade_checker_output() : m_console(mysqlsh::current_console()) {}

  void check_info(const std::string &server_addres,
                  const std::string &server_version,
                  const std::string &target_version) override {
    print_paragraph(
        shcore::str_format("The MySQL server at %s, version %s, will now be "
                           "checked for compatibility "
                           "issues for upgrade to MySQL %s...",
                           server_addres.c_str(), server_version.c_str(),
                           target_version.c_str()),
        0, 0);
  }

  void check_results(const Upgrade_check &check,
                     const std::vector<Upgrade_issue> &results) override {
    print_title(check.get_title());

    std::function<std::string(const Upgrade_issue &)> issue_formater(to_string);
    if (results.empty()) {
      print_paragraph("No issues found");
    } else if (check.get_description() != nullptr) {
      print_paragraph(check.get_description());
      print_doc_links(check.get_doc_link());
      m_console->println();
    } else {
      issue_formater = format_upgrade_issue;
    }

    for (const auto &issue : results) print_paragraph(issue_formater(issue));
  }

  void check_error(const Upgrade_check &check, const char *description,
                   bool runtime_error = true) override {
    print_title(check.get_title());
    m_console->print("  ");
    if (runtime_error) m_console->print_diag("Check failed: ");
    m_console->println(description);
    print_doc_links(check.get_doc_link());
  }

  void manual_check(const Upgrade_check &check) override {
    print_title(check.get_title());
    print_paragraph(check.get_description());
    print_doc_links(check.get_doc_link());
  }

  void summarize(int error, int warning, int notice,
                 const std::string &text) override {
    m_console->println();
    m_console->println(shcore::str_format("Errors:   %d", error));
    m_console->println(shcore::str_format("Warnings: %d", warning));
    m_console->println(shcore::str_format("Notices:  %d\n", notice));
    m_console->println(text);
  }

 private:
  void print_title(const char *title) {
    m_console->println();
    print_paragraph(shcore::str_format("%d) %s", ++m_check_count, title), 0, 0);
  }

  void print_paragraph(const std::string &s, std::size_t base_indent = 2,
                       std::size_t indent_by = 2) {
    std::string indent(base_indent, ' ');
    auto descr = shcore::str_break_into_lines(s, 80 - base_indent);
    for (std::size_t i = 0; i < descr.size(); i++) {
      m_console->println(indent + descr[i]);
      if (i == 0) indent.append(std::string(indent_by, ' '));
    }
  }

  void print_doc_links(const char *links) {
    if (links != nullptr) {
      std::string docs("More information:\n");
      print_paragraph(docs + links);
    }
  }

  int m_check_count = 0;
  std::shared_ptr<IConsole> m_console;
};

class JSON_upgrade_checker_output : public Upgrade_check_output_formatter {
 public:
  JSON_upgrade_checker_output()
      : m_json_document(),
        m_allocator(m_json_document.GetAllocator()),
        m_checks(rapidjson::kArrayType),
        m_manual_checks(rapidjson::kArrayType) {
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

  void check_error(const Upgrade_check &check, const char *description,
                   bool runtime_error = true) override {
    rapidjson::Value check_object(rapidjson::kObjectType);
    rapidjson::Value id;
    check_object.AddMember("id", rapidjson::StringRef(check.get_name()),
                           m_allocator);
    check_object.AddMember("title", rapidjson::StringRef(check.get_title()),
                           m_allocator);
    if (runtime_error)
      check_object.AddMember("status", rapidjson::StringRef("ERROR"),
                             m_allocator);
    else
      check_object.AddMember(
          "status", rapidjson::StringRef("CONFIGURATION_ERROR"), m_allocator);

    rapidjson::Value descr;
    descr.SetString(description, strlen(description), m_allocator);
    check_object.AddMember("description", descr, m_allocator);
    if (check.get_doc_link() != nullptr)
      check_object.AddMember("documentationLink",
                             rapidjson::StringRef(check.get_doc_link()),
                             m_allocator);

    rapidjson::Value issues(rapidjson::kArrayType);
    m_checks.PushBack(check_object, m_allocator);
  }

  void manual_check(const Upgrade_check &check) override {
    rapidjson::Value check_object(rapidjson::kObjectType);
    rapidjson::Value id;
    check_object.AddMember("id", rapidjson::StringRef(check.get_name()),
                           m_allocator);
    check_object.AddMember("title", rapidjson::StringRef(check.get_title()),
                           m_allocator);

    check_object.AddMember("description",
                           rapidjson::StringRef(check.get_description()),
                           m_allocator);
    if (check.get_doc_link() != nullptr)
      check_object.AddMember("documentationLink",
                             rapidjson::StringRef(check.get_doc_link()),
                             m_allocator);
    m_manual_checks.PushBack(check_object, m_allocator);
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
    m_json_document.AddMember("manualChecks", m_manual_checks, m_allocator);

    rapidjson::StringBuffer buffer;
    if (mysqlsh::current_shell_options()->get().wrap_json == "json/raw") {
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      m_json_document.Accept(writer);
    } else {
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      m_json_document.Accept(writer);
    }
    mysqlsh::current_console()->raw_print(
        buffer.GetString(), mysqlsh::Output_stream::STDOUT, false);
    mysqlsh::current_console()->raw_print("\n", mysqlsh::Output_stream::STDOUT,
                                          false);
  }

 private:
  rapidjson::Document m_json_document;
  rapidjson::Document::AllocatorType &m_allocator;
  rapidjson::Value m_checks;
  rapidjson::Value m_manual_checks;
};

std::unique_ptr<Upgrade_check_output_formatter>
Upgrade_check_output_formatter::get_formatter(const std::string &format) {
  if (shcore::str_casecmp(format, "JSON") == 0)
    return std::unique_ptr<Upgrade_check_output_formatter>(
        new JSON_upgrade_checker_output());
  else if (shcore::str_casecmp(format, "TEXT") != 0)
    throw std::invalid_argument(
        "Allowed values for outputFormat parameter are TEXT or JSON");

  return std::unique_ptr<Upgrade_check_output_formatter>(
      new Text_upgrade_checker_output());
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
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL,
              "If no connectionData is specified tool will try to establish "
              "connection using data from current session.");
REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL1,
              "Tool behaviour can be modified with following options:");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL2,
              "@li configPath - full path to MySQL server configuration file.");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL3,
              "@li outputFormat - value can be either TEXT (default) or JSON.");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL4,
              "@li targetVersion - version to which upgrade will be checked "
              "(default=" MYSH_VERSION ")");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL5,
              "@li password - password for connection.");

REGISTER_HELP(UTIL_CHECKFORSERVERUPGRADE_DETAIL6, "${TOPIC_CONNECTION_DATA}");

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
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL4)
 * $(UTIL_CHECKFORSERVERUPGRADE_DETAIL5)
 *
 * \copydoc connection_options
 *
 * Detailed description of the connection data format is available at \ref
 * connection_data
 *
 */
#if DOXYGEN_JS
Undefined Util::checkForServerUpgrade(ConnectionData connectionData,
                                      Dictionary options);
#elif DOXYGEN_PY
None Util::check_for_server_upgrade(ConnectionData connectionData,
                                    dict options);
#endif

shcore::Value Util::check_for_server_upgrade(
    const shcore::Argument_list &args) {
  args.ensure_count(0, 2, get_function_name("checkForServerUpgrade").c_str());
  int errors = 0, warnings = 0, notices = 0;

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

    using mysqlshdk::utils::Version;
    Upgrade_check_options opts{Version(), Version(MYSH_VERSION), "", ""};
    std::string output_format(
        shcore::str_beginswith(
            mysqlsh::current_shell_options()->get().wrap_json, "json")
            ? "JSON"
            : "TEXT");

    if (args.size() > 0 &&
        args[args.size() - 1].type == shcore::Value_type::Map) {
      auto dict = args.map_at(args.size() - 1);
      output_format = dict->get_string("outputFormat", output_format);
      auto target_version = dict->get_string("targetVersion", MYSH_VERSION);
      if (target_version == "8.0")
        opts.target_version = Version(MYSH_VERSION);
      else
        opts.target_version = Version(target_version);
      opts.config_path = dict->get_string("configPath", "");
    }

    auto print = Upgrade_check_output_formatter::get_formatter(output_format);

    auto session = establish_session(connection_options,
                                     current_shell_options()->get().wizards);

    auto result = session->query("select current_user();");
    const mysqlshdk::db::IRow *row = result->fetch_one();
    if (row == nullptr)
      throw std::runtime_error("Unable to get information about a user");

    std::string user;
    std::string host;
    shcore::split_account(row->get_string(0), &user, &host, true);
    if (user != "skip-grants user" && host != "skip-grants host") {
      mysqlshdk::mysql::Instance instance(session);
      auto res = instance.get_user_privileges(user, host)->validate({"all"});
      if (res.has_missing_privileges())
        throw std::invalid_argument(
            "The upgrade check needs to be performed by user with ALL "
            "privileges.");
    }

    auto version_result = session->query(
        "select @@version, @@version_comment, UPPER(@@version_compile_os);");
    row = version_result->fetch_one();
    if (row == nullptr)
      throw std::runtime_error("Unable to get server version");

    opts.server_version = Version(row->get_string(0));
    opts.server_os = row->get_string(2);

    print->check_info(connection_options.as_uri(
                          mysqlshdk::db::uri::formats::only_transport()),
                      row->get_string(0) + " - " + row->get_string(1),
                      opts.target_version.get_base());

    auto checklist = Upgrade_check::create_checklist(opts);

    const auto update_counts = [&errors, &warnings,
                                &notices](Upgrade_issue::Level level) {
      switch (level) {
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
    };

    for (auto &check : checklist)
      if (check->is_runnable()) {
        try {
          std::vector<Upgrade_issue> issues = check->run(session, opts);
          for (const auto &issue : issues) update_counts(issue.level);
          print->check_results(*check, issues);
        } catch (const Upgrade_check::Check_configuration_error &e) {
          print->check_error(*check, e.what(), false);
        } catch (const std::exception &e) {
          print->check_error(*check, e.what());
        }
      } else {
        update_counts(check->get_level());
        print->manual_check(*check);
      }

    std::string summary;
    if (errors > 0) {
      summary = shcore::str_format(
          "%i errors were found. Please correct these issues before upgrading "
          "to avoid compatibility issues.",
          errors);
    } else if (warnings > 0) {
      summary =
          "No fatal errors were found that would prevent an upgrade, "
          "but some potential issues were detected. Please ensure that the "
          "reported issues are not significant before upgrading.";
    } else if (notices > 0) {
      summary =
          "No fatal errors were found that would prevent an upgrade, "
          "but some potential issues were detected. Please ensure that the "
          "reported issues are not significant before upgrading.";
    } else {
      summary = "No known compatibility errors or issues were found.";
    }
    print->summarize(errors, warnings, notices, summary);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(
      get_function_name("checkForServerUpgrade"));

  return shcore::Value();
}

REGISTER_HELP_FUNCTION(importJson, util);
REGISTER_HELP(UTIL_IMPORTJSON_BRIEF,
              "Import JSON documents from file to collection or table in MySQL "
              "Server using X Protocol session.");

REGISTER_HELP(UTIL_IMPORTJSON_PARAM, "@param file Path to JSON documents file");

REGISTER_HELP(UTIL_IMPORTJSON_PARAM1,
              "@param options Optional dictionary with import options");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL,
              "This function reads standard JSON documents from a file, "
              "however, it also supports converting BSON Data Types "
              "represented using the MongoDB Extended Json (strict mode) into "
              "MySQL values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL1,
              "The options dictionary supports the following options:");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL2,
              "@li schema: string - name of target schema.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL3,
              "@li collection: string - name of collection where the data will "
              "be imported.");
REGISTER_HELP(
    UTIL_IMPORTJSON_DETAIL4,
    "@li table: string - name of table where the data will be imported.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL5,
              "@li tableColumn: string (default: \"doc\") - name of column in "
              "target table where the imported JSON documents will be stored.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL6,
              "@li convertBsonTypes: bool (default: false) - enables the BSON "
              "data type conversion.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL7,
              "@li convertBsonOid: bool (default: the value of "
              "convertBsonTypes) - enables conversion of the BSON ObjectId "
              "values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL8,
              "@li extractOidTime: string (default: empty) - creates a new "
              "field based on the ObjectID timestamp. Only valid if "
              "convertBsonOid is enabled.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL9,
              "The following options are valid only when convertBsonTypes is "
              "enabled. They are all boolean flags. ignoreRegexOptions is "
              "enabled by default, rest are disabled by default.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL10,
              "@li ignoreDate: disables conversion of BSON Date values");
REGISTER_HELP(
    UTIL_IMPORTJSON_DETAIL11,
    "@li ignoreTimestamp: disables conversion of BSON Timestamp values");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL12,
              "@li ignoreRegex: disables conversion of BSON Regex values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL15,
              "@li ignoreRegexOptions: causes regex options to be ignored when "
              "processing a Regex BSON value. This option is only valid if "
              "ignoreRegex is disabled.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL13,
              "@li ignoreBinary: disables conversion of BSON BinData values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL14,
              "@li decimalAsDouble: causes BSON Decimal values to be imported "
              "as double values.");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL16,
              "If the schema is not provided, an active schema on the global "
              "session, if set, will be used.");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL17,
              "The collection and the table options cannot be combined. If "
              "they are not provided, the basename of the file without "
              "extension will be used as target collection name.");

REGISTER_HELP(
    UTIL_IMPORTJSON_DETAIL18,
    "If the target collection or table does not exist, they are created, "
    "otherwise the data is inserted into the existing collection or table.");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL19,
              "The tableColumn implies the use of the table option and cannot "
              "be combined "
              "with the collection option.");

REGISTER_HELP(UTIL_IMPORTJSON_DETAIL20, "<b>BSON Data Type Processing.</b>");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL21,
              "If only convertBsonOid is enabled, no conversion will be done "
              "on the rest of the BSON Data Types.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL22,
              "To use extractOidTime, it should be set to a name which will "
              "be used to insert an additional field into the main document. "
              "The value of the new field will be the timestamp obtained from "
              "the ObjectID value. Note that this will be done only for an "
              "ObjectID value associated to the '_id' field of the main "
              "document.");
REGISTER_HELP(
    UTIL_IMPORTJSON_DETAIL23,
    "NumberLong and NumberInt values will be converted to integer values.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL24,
              "NumberDecimal values are imported as strings, unless "
              "decimalAsDouble is enabled.");
REGISTER_HELP(UTIL_IMPORTJSON_DETAIL25,
              "Regex values will be converted to strings containing the "
              "regular expression. The regular expression options are ignored "
              "unless ignoreRegexOptions is disabled. When ignoreRegexOptions "
              "is disabled the regular expression will be converted to the "
              "form: /@<regex@>/@<options@>.");

REGISTER_HELP(UTIL_IMPORTJSON_THROWS, "Throws ArgumentError when:");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS1, "@li Option name is invalid");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS2,
              "@li Required options are not set and cannot be deduced");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS3,
              "@li Shell is not connected to MySQL Server using X Protocol");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS4,
              "@li Schema is not provided and there is no active schema on the "
              "global session");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS5,
              "@li Both collection and table are specified");

REGISTER_HELP(UTIL_IMPORTJSON_THROWS6, "Throws LogicError when:");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS7,
              "@li Path to JSON document does not exists or is not a file");

REGISTER_HELP(UTIL_IMPORTJSON_THROWS8, "Throws RuntimeError when:");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS9, "@li The schema does not exists");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS10, "@li MySQL Server returns an error");

REGISTER_HELP(UTIL_IMPORTJSON_THROWS11, "Throws InvalidJson when:");
REGISTER_HELP(UTIL_IMPORTJSON_THROWS12, "@li JSON document is ill-formed");

/**
 * \ingroup util
 *
 * $(UTIL_IMPORTJSON_BRIEF)
 *
 * $(UTIL_IMPORTJSON_PARAM)
 * $(UTIL_IMPORTJSON_PARAM1)
 *
 * $(UTIL_IMPORTJSON_DETAIL)
 *
 * $(UTIL_IMPORTJSON_DETAIL1)
 * $(UTIL_IMPORTJSON_DETAIL2)
 * $(UTIL_IMPORTJSON_DETAIL3)
 * $(UTIL_IMPORTJSON_DETAIL4)
 * $(UTIL_IMPORTJSON_DETAIL5)
 * $(UTIL_IMPORTJSON_DETAIL6)
 * $(UTIL_IMPORTJSON_DETAIL7)
 * $(UTIL_IMPORTJSON_DETAIL8)
 *
 * $(UTIL_IMPORTJSON_DETAIL9)
 * $(UTIL_IMPORTJSON_DETAIL10)
 * $(UTIL_IMPORTJSON_DETAIL11)
 * $(UTIL_IMPORTJSON_DETAIL12)
 * $(UTIL_IMPORTJSON_DETAIL13)
 * $(UTIL_IMPORTJSON_DETAIL14)
 * $(UTIL_IMPORTJSON_DETAIL15)
 *
 * $(UTIL_IMPORTJSON_DETAIL16)
 *
 * $(UTIL_IMPORTJSON_DETAIL17)
 *
 * $(UTIL_IMPORTJSON_DETAIL18)
 *
 * $(UTIL_IMPORTJSON_DETAIL19)
 *
 * $(UTIL_IMPORTJSON_DETAIL20)
 *
 * $(UTIL_IMPORTJSON_DETAIL21)
 *
 * $(UTIL_IMPORTJSON_DETAIL22)
 *
 * $(UTIL_IMPORTJSON_DETAIL23)
 *
 * $(UTIL_IMPORTJSON_DETAIL24)
 *
 * $(UTIL_IMPORTJSON_DETAIL25)
 *
 * $(UTIL_IMPORTJSON_THROWS)
 * $(UTIL_IMPORTJSON_THROWS1)
 * $(UTIL_IMPORTJSON_THROWS2)
 * $(UTIL_IMPORTJSON_THROWS3)
 * $(UTIL_IMPORTJSON_THROWS4)
 * $(UTIL_IMPORTJSON_THROWS5)
 *
 * $(UTIL_IMPORTJSON_THROWS6)
 * $(UTIL_IMPORTJSON_THROWS7)
 *
 * $(UTIL_IMPORTJSON_THROWS8)
 * $(UTIL_IMPORTJSON_THROWS9)
 * $(UTIL_IMPORTJSON_THROWS10)
 *
 * $(UTIL_IMPORTJSON_THROWS11)
 * $(UTIL_IMPORTJSON_THROWS12)
 */
#if DOXYGEN_JS
Undefined Util::importJson(String file, Dictionary options);
#elif DOXYGEN_PY
None Util::import_json(str file, dict options);
#endif

void Util::import_json(const std::string &file,
                       const shcore::Dictionary_t &options) {
  std::string schema;
  std::string collection;
  std::string table;
  std::string table_column;

  shcore::Option_unpacker unpacker(options);
  unpacker.optional("schema", &schema);
  unpacker.optional("collection", &collection);
  unpacker.optional("table", &table);
  unpacker.optional("tableColumn", &table_column);

  shcore::Document_reader_options roptions;
  mysqlsh::unpack_json_import_flags(&unpacker, &roptions);

  unpacker.end();

  if (!roptions.extract_oid_time.is_null() &&
      (*roptions.extract_oid_time).empty()) {
    throw shcore::Exception::runtime_error(
        "Option 'extractOidTime' can not be empty.");
  }

  auto shell_session = _shell_core.get_dev_session();

  if (!shell_session) {
    throw shcore::Exception::runtime_error(
        "Please connect the shell to the MySQL server.");
  }

  auto node_type = shell_session->get_node_type();
  if (node_type.compare("X") != 0) {
    throw shcore::Exception::runtime_error(
        "An X Protocol session is required for JSON import.");
  }

  Connection_options connection_options =
      shell_session->get_connection_options();

  std::shared_ptr<mysqlshdk::db::mysqlx::Session> xsession =
      mysqlshdk::db::mysqlx::Session::create();

  if (current_shell_options()->get().trace_protocol) {
    xsession->enable_protocol_trace(true);
  }
  xsession->connect(connection_options);

  Prepare_json_import prepare{xsession};

  if (!schema.empty()) {
    prepare.schema(schema);
  } else if (!shell_session->get_current_schema().empty()) {
    prepare.schema(shell_session->get_current_schema());
  } else {
    throw std::runtime_error(
        "There is no active schema on the current session, the target schema "
        "for the import operation must be provided in the options.");
  }

  prepare.path(file);

  if (!table.empty()) {
    prepare.table(table);
  }

  if (!table_column.empty()) {
    prepare.column(table_column);
  }

  if (!collection.empty()) {
    if (!table_column.empty()) {
      throw std::invalid_argument(
          "tableColumn cannot be used with collection.");
    }
    prepare.collection(collection);
  }

  // Validate provided parameters and build Json_importer object.
  auto importer = prepare.build();

  auto console = mysqlsh::current_console();
  console->print_info(
      prepare.to_string() + " in MySQL Server at " +
      connection_options.as_uri(mysqlshdk::db::uri::formats::only_transport()) +
      "\n");

  importer.set_print_callback([](const std::string &msg) -> void {
    mysqlsh::current_console()->print(msg);
  });

  try {
    importer.load_from(roptions);
  } catch (...) {
    importer.print_stats();
    throw;
  }
  importer.print_stats();
}

#ifdef WITH_OCI
REGISTER_HELP_TOPIC(OCI, TOPIC, TOPIC_OCI, Contents, ALL);
REGISTER_HELP_TOPIC_TEXT(TOPIC_OCI, R"*(
The MySQL Shell offers support for the Oracle Cloud Infrastructure (OCI).

After starting the MySQL Shell with the --oci option an interactive wizard will help to create the correct OCI configuration file, load the OCI Python SDK and switch the shell to Python mode.

The MySQL Shell can then be used to access the OCI APIs and manage the OCI account.

The following Python objects are automatically initialized.

@li <b>config</b> an OCI configuration object with data loaded from ~/.oci/config
@li <b>identity</b> an OCI identity client object, offering APIs for managing users,
              groups, compartments and policies.
@li <b>compute</b> an OCI compute client object, offering APIs for Networking Service, Compute Service, and Block Volume Service.

For more information about the OCI Python SDK please read the documentation at
  https://oracle-cloud-infrastructure-python-sdk.readthedocs.io/en/latest/
)*");

REGISTER_HELP(TOPIC_OCI_EXAMPLE, "identity.get_user(config['user']).data");
REGISTER_HELP(TOPIC_OCI_EXAMPLE_DESC,
              "Fetches information about the OCI user account specified in the "
              "config object.");
REGISTER_HELP(TOPIC_OCI_EXAMPLE1,
              "identity.list_compartments(config['tenancy']).data");
REGISTER_HELP(TOPIC_OCI_EXAMPLE1_DESC,
              "Fetches the list of top level compartments available in the "
              "tenancy that was specified in the config object.");
REGISTER_HELP(
    TOPIC_OCI_EXAMPLE2,
    "@code"
    "compartment = identity.list_compartments(config['tenancy']).data[0]\n"
    "images = compute.list_images(compartment.id).data\n"
    "for image in images:\n"
    "  print(image.display_name)"
    "@endcode");
REGISTER_HELP(TOPIC_OCI_EXAMPLE2_DESC,
              "Assignes the first compartment of the tenancy to the "
              "compartment variable, featches the available OS images for the "
              "compartment and prints a list of their names.");

REGISTER_HELP_FUNCTION(configureOci, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_CONFIGUREOCI, R"*(
Wizard to create a valid configuration for the OCI SDK.

@param profile Optional parameter to specify the name profile to be configured.

If the profile name is not specified 'DEFAULT' will be used.

To properly create OCI configuration to use the OCI SDK the following
information will be required:

@li User OCID
@li Tenancy OCID
@li Tenancy region
@li A valid API key

For details about where to find the user and tenancy details go to
https://docs.us-phoenix-1.oraclecloud.com/Content/API/Concepts/apisigningkey.htm#Other
)*");
/**
 * $(UTIL_CONFIGUREOCI_BRIEF)
 *
 * $(UTIL_CONFIGUREOCI)
 */
#if DOXYGEN_JS
Undefined Util::configureOci(String profile) {}
#elif DOXYGEN_PY
None Util::configure_oci(str profile) {}
#endif
void Util::configure_oci(const std::string &profile) {
  oci::Oci_setup helper;

  helper.create_profile(profile);
}
#endif

REGISTER_HELP_FUNCTION(importTable, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_IMPORTTABLE, R"*(
Import table dump stored in filename to target table using LOAD DATA LOCAL
INFILE calls in parallel connections.

@param filename Path to file with user data
@param options Optional dictionary with import options

Scheme part of <b>filename</b> contains infomation about the transport backend.
Supported transport backends are: file://, http://, https://, oci+os://.
If scheme part of <b>filename</b> is omitted, then file:// transport
backend will be chosen.

Supported filename formats:
@li <b>[file://]/path/to/file</b> - Read import data from local file
@li <b>http[s]://host.domain[:port]/path/to/file</b> - Read import data from
file provided in URL
@li <b>oci+os://namespace/region/bucket/object</b> - Read import data from
object stored in OCI (Oracle Cloud Infrastructure) Object Storage. Variables
needed to sign requests will be obtained from profile configured in OCI
configuration file. Profile name and configuration file path are specified
in oci.profile and oci.configFile shell options.
ociProfile and ociConfigFile options will override, respectively,
oci.profile and oci.configFile shell options.

Options dictionary:
@li <b>schema</b>: string (default: current shell active schema) - Name of
target schema
@li <b>table</b>: string (default: filename without extension) - Name of target
table
@li <b>columns</b>: string array (default: empty array) - This option takes a
array of column names as its value. The order of the column names indicates how
to match data file columns with table columns.
@li <b>fieldsTerminatedBy</b>: string (default: "\t"), <b>fieldsEnclosedBy</b>:
char (default: ''), <b>fieldsEscapedBy</b>: char (default: '\\') - These options
have the same meaning as the corresponding clauses for LOAD DATA INFILE. For
more information use <b>\\? LOAD DATA</b>, (a session is required).
@li <b>fieldsOptionallyEnclosed</b>: bool (default: false) - Set to true if the
input values are not necessarily enclosed within quotation marks specified by
<b>fieldsEnclosedBy</b> option. Set to false if all fields are quoted by
character specified by <b>fieldsEnclosedBy</b> option.
@li <b>linesTerminatedBy</b>: string (default: "\n") - This option has the same
meaning as the corresponding clause for LOAD DATA INFILE. For example, to import
Windows files that have lines terminated with carriage return/linefeed pairs,
use --lines-terminated-by="\r\n". (You might have to double the backslashes,
depending on the escaping conventions of your command interpreter.)
See Section 13.2.7, "LOAD DATA INFILE Syntax".
@li <b>replaceDuplicates</b>: bool (default: false) - If true, input rows that
have the same value for a primary key or unique index as an existing row will be
replaced, otherwise input rows will be skipped.
@li <b>threads</b>: int (default: 8) - Use N threads to sent file chunks to the
server.
@li <b>bytesPerChunk</b>: string (minimum: "131072", default: "50M") - Send
bytesPerChunk (+ bytes to end of the row) in single LOAD DATA call. Unit
suffixes, k - for Kilobytes (n * 1'000 bytes), M - for Megabytes (n * 1'000'000
bytes), G - for Gigabytes (n * 1'000'000'000 bytes), bytesPerChunk="2k" - ~2
kilobyte data chunk will send to the MySQL Server.
@li <b>maxRate</b>: string (default: "0") - Limit data send throughput to
maxRate in bytes per second per thread.
maxRate="0" - no limit. Unit suffixes, k - for Kilobytes (n * 1'000 bytes),
M - for Megabytes (n * 1'000'000 bytes), G - for Gigabytes (n * 1'000'000'000
bytes), maxRate="2k" - limit to 2 kilobytes per second.
@li <b>showProgress</b>: bool (default: true if stdout is a tty, false
otherwise) - Enable or disable import progress information.
@li <b>skipRows</b>: int (default: 0) - Skip first n rows of the data in the
file. You can use this option to skip an initial header line containing column
names.
@li <b>dialect</b>: enum (default: "default") - Setup fields and lines options
that matches specific data file format. Can be used as base dialect and
customized with fieldsTerminatedBy, fieldsEnclosedBy, fieldsOptionallyEnclosed,
fieldsEscapedBy and linesTerminatedBy options. Must be one of the following
values: csv, tsv, json or csv-unix.
@li <b>ociConfigFile</b>: string (default: not set) - Override oci.configFile
shell option. Available only if oci+os:// transport protocol is in use.
@li <b>ociProfile</b>: string (default: not set) - Override oci.profile shell
option. Available only if oci+os:// transport protocol is in use.

<b>dialect</b> predefines following set of options fieldsTerminatedBy (FT),
fieldsEnclosedBy (FE), fieldsOptionallyEnclosed (FOE), fieldsEscapedBy (FESC)
and linesTerminatedBy (LT) in following manner:
@li default: no quoting, tab-separated, lf line endings.
(LT=@<LF@>, FESC='\', FT=@<TAB@>, FE=@<empty@>, FOE=false)
@li csv: optionally quoted, comma-separated, crlf line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=",", FE='"', FOE=true)
@li tsv: optionally quoted, tab-separated, crlf line endings.
(LT=@<CR@>@<LF@>, FESC='\', FT=@<TAB@>, FE='"', FOE=true)
@li json: one JSON document per line.
(LT=@<LF@>, FESC=@<empty@>, FT=@<LF@>, FE=@<empty@>, FOE=false)
@li csv-unix: fully quoted, comma-separated, lf line endings.
(LT=@<LF@>, FESC='\', FT=",", FE='"', FOE=false)

Example input data for dialects:
@li default:
@code
1<TAB>20.1000<TAB>foo said: "Where is my bar?"<LF>
2<TAB>-12.5000<TAB>baz said: "Where is my \<TAB> char?"<LF>
@endcode
@li csv:
@code
1,20.1000,"foo said: \"Where is my bar?\""<CR><LF>
2,-12.5000,"baz said: \"Where is my <TAB> char?\""<CR><LF>
@endcode
@li tsv:
@code
1<TAB>20.1000<TAB>"foo said: \"Where is my bar?\""<CR><LF>
2<TAB>-12.5000<TAB>"baz said: \"Where is my <TAB> char?\""<CR><LF>
@endcode
@li json:
@code
{"id_int": 1, "value_float": 20.1000, "text_text": "foo said: \"Where is my bar?\""}<LF>
{"id_int": 2, "value_float": -12.5000, "text_text": "baz said: \"Where is my \u000b char?\""}<LF>
@endcode
@li csv-unix:
@code
"1","20.1000","foo said: \"Where is my bar?\""<LF>
"2","-12.5000","baz said: \"Where is my <TAB> char?\""<LF>
@endcode

If the <b>schema</b> is not provided, an active schema on the global session, if
set, will be used.

If the input values are not necessarily enclosed within <b>fieldsEnclosedBy</b>,
set <b>fieldsOptionallyEnclosed</b> to true.

If you specify one separator that is the same as or a prefix of another, LOAD
DATA INFILE cannot interpret the input properly.

Connection options set in the global session, such as compression, ssl-mode, etc.
are used in parallel connections.

Each parallel connection sets the following session variables:
@li SET unique_checks = 0
@li SET foreign_key_checks = 0
@li SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED
)*");
// clang-format off
/**
 * \ingroup util
 *
 * Import table dump stored in filename to target table using LOAD DATA LOCAL
 * INFILE calls in parallel connections.
 *
 * @param filename Path to file with user data
 * @param options Optional dictionary with import options
 *
 * Scheme part of <b>filename</b> contains infomation about the transport backend.
 * Supported transport backends are: file://, http://, https://, oci+os://.
 * If scheme part of <b>filename</b> is omitted, then file:// transport
 * backend will be chosen.
 *
 * Supported filename formats:
 * @li <b>[file://]/path/to/file</b> - Read import data from local file
 * @li <b>http[s]://host.domain[:port]/path/to/file</b> - Read import data from
 * file provided in URL
 * @li <b>oci+os://namespace/region/bucket/object</b> - Read import data from
 * object stored in OCI (Oracle Cloud Infrastructure) Object Storage. Variables
 * needed to sign requests will be obtained from profile configured in OCI
 * configuration file. Profile name and configuration file path are specified
 * in oci.profile and oci.configFile shell options.
 * ociProfile and ociConfigFile options will override, respectively,
 * oci.profile and oci.configFile shell options.
 *
 * Options dictionary:
 * @li <b>schema</b>: string (default: current shell active schema) - Name of
 * target schema
 * @li <b>table</b>: string (default: filename without extension) - Name of target
 * table
 * @li <b>columns</b>: string array (default: empty array) - This option takes a
 * array of column names as its value. The order of the column names indicates how
 * to match data file columns with table columns.
 * @li <b>fieldsTerminatedBy</b>: string (default: "\t"), <b>fieldsEnclosedBy</b>:
 * char (default: ''), <b>fieldsEscapedBy</b>: char (default: '\\') - These options
 * have the same meaning as the corresponding clauses for LOAD DATA INFILE. For
 * more information use <b>\\? LOAD DATA</b>, (a session is required).
 * @li <b>fieldsOptionallyEnclosed</b>: bool (default: false) - Set to true if the
 * input values are not necessarily enclosed within quotation marks specified by
 * <b>fieldsEnclosedBy</b> option. Set to false if all fields are quoted by
 * character specified by <b>fieldsEnclosedBy</b> option.
 * @li <b>linesTerminatedBy</b>: string (default: "\n") - This option has the same
 * meaning as the corresponding clause for LOAD DATA INFILE. For example, to import
 * Windows files that have lines terminated with carriage return/linefeed pairs,
 * use --lines-terminated-by="\r\n". (You might have to double the backslashes,
 * depending on the escaping conventions of your command interpreter.)
 * See Section 13.2.7, "LOAD DATA INFILE Syntax".
 * @li <b>replaceDuplicates</b>: bool (default: false) - If true, input rows that
 * have the same value for a primary key or unique index as an existing row will be
 * replaced, otherwise input rows will be skipped.
 * @li <b>threads</b>: int (default: 8) - Use N threads to sent file chunks to the
 * server.
 * @li <b>bytesPerChunk</b>: string (minimum: "131072", default: "50M") - Send
 * bytesPerChunk (+ bytes to end of the row) in single LOAD DATA call. Unit
 * suffixes, k - for Kilobytes (n * 1'000 bytes), M - for Megabytes (n * 1'000'000
 * bytes), G - for Gigabytes (n * 1'000'000'000 bytes), bytesPerChunk="2k" - ~2
 * kilobyte data chunk will send to the MySQL Server.
 * @li <b>maxRate</b>: string (default: "0") - Limit data send throughput to
 * maxRate in bytes per second per thread.
 * maxRate="0" - no limit. Unit suffixes, k - for Kilobytes (n * 1'000 bytes),
 * M - for Megabytes (n * 1'000'000 bytes), G - for Gigabytes (n * 1'000'000'000
 * bytes), maxRate="2k" - limit to 2 kilobytes per second.
 * @li <b>showProgress</b>: bool (default: true if stdout is a tty, false
 * otherwise) - Enable or disable import progress information.
 * @li <b>skipRows</b>: int (default: 0) - Skip first n rows of the data in the
 * file. You can use this option to skip an initial header line containing column
 * names.
 * @li <b>dialect</b>: enum (default: "default") - Setup fields and lines options
 * that matches specific data file format. Can be used as base dialect and
 * customized with fieldsTerminatedBy, fieldsEnclosedBy, fieldsOptionallyEnclosed,
 * fieldsEscapedBy and linesTerminatedBy options. Must be one of the following
 * values: csv, tsv, json or csv-unix.
 * @li <b>ociConfigFile</b>: string (default: not set) - Override oci.configFile
 * shell option. Available only if oci+os:// transport protocol is in use.
 * @li <b>ociProfile</b>: string (default: not set) - Override oci.profile shell
 * option. Available only if oci+os:// transport protocol is in use.
 *
 * <b>dialect</b> predefines following set of options fieldsTerminatedBy (FT),
 * fieldsEnclosedBy (FE), fieldsOptionallyEnclosed (FOE), fieldsEscapedBy (FESC)
 * and linesTerminatedBy (LT) in following manner:
 * @li default: no quoting, tab-separated, lf line endings.
 * (LT=@<LF@>, FESC='\', FT=@<TAB@>, FE=@<empty@>, FOE=false)
 * @li csv: optionally quoted, comma-separated, crlf line endings.
 * (LT=@<CR@>@<LF@>, FESC='\', FT=",", FE='"', FOE=true)
 * @li tsv: optionally quoted, tab-separated, crlf line endings.
 * (LT=@<CR@>@<LF@>, FESC='\', FT=@<TAB@>, FE='"', FOE=true)
 * @li json: one JSON document per line.
 * (LT=@<LF@>, FESC=@<empty@>, FT=@<LF@>, FE=@<empty@>, FOE=false)
 * @li csv-unix: fully quoted, comma-separated, lf line endings.
 * (LT=@<LF@>, FESC='\', FT=",", FE='"', FOE=false)
 *
 * Example input data for dialects:
 * @li default:
 * @code{.unparsed}
 * 1<TAB>20.1000<TAB>foo said: "Where is my bar?"<LF>
 * 2<TAB>-12.5000<TAB>baz said: "Where is my \<TAB> char?"<LF>
 * @endcode
 * @li csv:
 * @code{.unparsed}
 * 1,20.1000,"foo said: \"Where is my bar?\""<CR><LF>
 * 2,-12.5000,"baz said: \"Where is my <TAB> char?\""<CR><LF>
 * @endcode
 * @li tsv:
 * @code{.unparsed}
 * 1<TAB>20.1000<TAB>"foo said: \"Where is my bar?\""<CR><LF>
 * 2<TAB>-12.5000<TAB>"baz said: \"Where is my <TAB> char?\""<CR><LF>
 * @endcode
 * @li json:
 * @code{.unparsed}
 * {"id_int": 1, "value_float": 20.1000, "text_text": "foo said: \"Where is my bar?\""}<LF>
 * {"id_int": 2, "value_float": -12.5000, "text_text": "baz said: \"Where is my \u000b char?\""}<LF>
 * @endcode
 * @li csv-unix:
 * @code{.unparsed}
 * "1","20.1000","foo said: \"Where is my bar?\""<LF>
 * "2","-12.5000","baz said: \"Where is my <TAB> char?\""<LF>
 * @endcode
 *
 * If the <b>schema</b> is not provided, an active schema on the global session, if
 * set, will be used.
 *
 * If the input values are not necessarily enclosed within <b>fieldsEnclosedBy</b>,
 * set <b>fieldsOptionallyEnclosed</b> to true.
 *
 * If you specify one separator that is the same as or a prefix of another, LOAD
 * DATA INFILE cannot interpret the input properly.
 *
 * Connection options set in the global session, such as compression, ssl-mode, etc.
 * are used in parallel connections.
 *
 * Each parallel connection sets the following session variables:
 * @li SET unique_checks = 0
 * @li SET foreign_key_checks = 0
 * @li SET SESSION TRANSACTION ISOLATION LEVEL READ UNCOMMITTED
 */
// clang-format on
#if DOXYGEN_JS
Undefined Util::importTable(String filename, Dictionary options);
#elif DOXYGEN_PY
None Util::import_table(str filename, dict options);
#endif
void Util::import_table(const std::string &filename,
                        const shcore::Dictionary_t &options) {
  using import_table::Import_table;
  using import_table::Import_table_options;
  using mysqlshdk::utils::format_bytes;

  Import_table_options opt(filename, options);
  opt.base_session(_shell_core.get_dev_session());
  opt.validate();

  volatile bool interrupt = false;
  shcore::Interrupt_handler intr_handler([&interrupt]() -> bool {
    mysqlsh::current_console()->print_warning(
        "Interrupted by user. Cancelling...");
    interrupt = true;
    return false;
  });

  Import_table importer(opt);
  importer.interrupt(&interrupt);

  auto console = mysqlsh::current_console();
  console->print_info(opt.target_import_info());

  importer.import();

  const auto filesize = opt.file_size();
  const bool thread_thrown_exception = importer.any_exception();
  if (thread_thrown_exception) {
    console->print_error("Error occur while importing file '" +
                         opt.full_path() + "' (" + format_bytes(filesize) +
                         ")");
  } else {
    console->print_info(importer.import_summary());
  }
  console->print_info(importer.rows_affected_info());
  importer.rethrow_exceptions();
}
}  // namespace mysqlsh

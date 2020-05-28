/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
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
#include "modules/util/dump/dump_instance.h"
#include "modules/util/dump/dump_instance_options.h"
#include "modules/util/dump/dump_schemas.h"
#include "modules/util/dump/dump_schemas_options.h"
#include "modules/util/import_table/import_table.h"
#include "modules/util/import_table/import_table_options.h"
#include "modules/util/json_importer.h"
#include "modules/util/load/dump_loader.h"
#include "modules/util/load/load_dump_options.h"
#include "modules/util/upgrade_check.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/oci/oci_setup.h"
#include "mysqlshdk/libs/utils/document_parser.h"
#include "mysqlshdk/libs/utils/profiling.h"
#include "mysqlshdk/libs/utils/ssl_keygen.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

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
  shcore::ssl::init();
  expose("configureOci", &Util::configure_oci, "?profile");
  expose("importTable", &Util::import_table, "path", "?options");
  expose("dumpSchemas", &Util::dump_schemas, "schemas", "outputUrl",
         "?options");
  expose("dumpInstance", &Util::dump_instance, "outputUrl", "?options");
  expose("loadDump", &Util::load_dump, "url", "?options");
}

namespace {
std::string format_upgrade_issue(const Upgrade_issue &problem) {
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
    if (!results.empty()) {
      if (check.get_description() != nullptr)
        check_object.AddMember("description",
                               rapidjson::StringRef(check.get_description()),
                               m_allocator);
      if (check.get_doc_link() != nullptr)
        check_object.AddMember("documentationLink",
                               rapidjson::StringRef(check.get_doc_link()),
                               m_allocator);
    }

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

void check_upgrade(const Connection_options &connection_options,
                   const shcore::Value::Map_type_ref &options) {
  using mysqlshdk::utils::Version;
  Upgrade_check_options opts{Version(), Version(MYSH_VERSION), "", ""};
  std::string output_format(
      shcore::str_beginswith(mysqlsh::current_shell_options()->get().wrap_json,
                             "json")
          ? "JSON"
          : "TEXT");

  if (options) {
    std::string target_version(MYSH_VERSION);
    std::string password;
    shcore::Option_unpacker(options)
        .optional("outputFormat", &output_format)
        .optional("targetVersion", &target_version)
        .optional("configPath", &opts.config_path)
        .optional("password", &password)
        .end();

    if (target_version == "8.0")
      opts.target_version = Version(MYSH_VERSION);
    else
      opts.target_version = Version(target_version);
  }

  auto print = Upgrade_check_output_formatter::get_formatter(output_format);

  auto session = establish_session(connection_options,
                                   current_shell_options()->get().wizards);

  auto result = session->query("select current_user();");
  const mysqlshdk::db::IRow *row = result->fetch_one();
  if (row == nullptr)
    throw std::runtime_error("Unable to get information about a user");

  try {
    std::string user;
    std::string host;
    shcore::split_account(row->get_string(0), &user, &host, true);
    if (user != "skip-grants user" && host != "skip-grants host") {
      mysqlshdk::mysql::Instance instance(session);
      auto res = instance.get_user_privileges(user, host)
                     ->validate({"PROCESS", "RELOAD", "SELECT"});
      if (res.has_missing_privileges())
        throw std::invalid_argument(
            "The upgrade check needs to be performed by user with RELOAD, "
            "PROCESS, and SELECT privileges.");
    }
  } catch (const std::runtime_error &e) {
    log_error("Unable to check permissions: %s", e.what());
  }

  auto version_result = session->query(
      "select @@version, @@version_comment, UPPER(@@version_compile_os);");
  row = version_result->fetch_one();
  if (row == nullptr) throw std::runtime_error("Unable to get server version");

  opts.server_version = Version(row->get_string(0));
  opts.server_os = row->get_string(2);

  print->check_info(
      connection_options.as_uri(mysqlshdk::db::uri::formats::only_transport()),
      row->get_string(0) + " - " + row->get_string(1),
      opts.target_version.get_base());

  auto checklist = Upgrade_check::create_checklist(opts);

  int errors = 0, warnings = 0, notices = 0;
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
}  // namespace

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
Undefined Util::checkForServerUpgrade(Dictionary options);
#elif DOXYGEN_PY
None Util::check_for_server_upgrade(ConnectionData connectionData,
                                    dict options);
None Util::check_for_server_upgrade(dict options);
#endif

shcore::Value Util::check_for_server_upgrade(
    const shcore::Argument_list &args) {
  args.ensure_count(0, 2, get_function_name("checkForServerUpgrade").c_str());

  try {
    Connection_options connection_options;
    std::shared_ptr<shcore::Value::Map_type> options_dictionary =
        args.size() > 0 && args[args.size() - 1].type == shcore::Value_type::Map
            ? args.map_at(args.size() - 1)
            : nullptr;

    if (args.size() > 0 && args[0] != shcore::Value::Null()) {
      try {
        connection_options = mysqlsh::get_connection_options(args[0]);
        if (args.size() > 1 && options_dictionary)
          set_password_from_map(&connection_options, options_dictionary);
        else
          options_dictionary.reset();
      } catch (const std::exception &) {
        if (!_shell_core.get_dev_session() || !options_dictionary ||
            args.size() == 2)
          throw;
        connection_options =
            _shell_core.get_dev_session()->get_connection_options();
      }
    } else {
      if (!_shell_core.get_dev_session())
        throw shcore::Exception::argument_error(
            "Please connect the shell to the MySQL server to be checked or "
            "specify the server URI as a parameter.");
      connection_options =
          _shell_core.get_dev_session()->get_connection_options();
    }

    if (args.size() == 2 && !options_dictionary)
      throw shcore::Exception::argument_error(
          "Argument #2 is expected to be a map.");

    check_upgrade(connection_options, options_dictionary);
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
  mysqlshdk::oci::Oci_setup helper;

  helper.create_profile(profile);
}

REGISTER_HELP_DETAIL_TEXT(IMPORT_EXPORT_URL_DETAIL, R"*(
<b>url</b> can be one of:
@li <b>/path/to/file</b> - Path to a locally or remotely (e.g. in OCI Object
Storage) accessible file or directory
@li <b>file:///path/to/file</b> - Path to a locally accessible file or directory
@li <b>http[s]://host.domain[:port]/path/to/file</b> - Location of a remote file
accessible through HTTP(s) (<<<importTable>>>() only)

If the <b>osBucketName</b> option is given, the path argument must specify a
plain path in that OCI (Oracle Cloud Infrastructure) Object Storage bucket.

The OCI configuration profile is located through the oci.profile and
oci.configFile global shell options and can be overriden with ociProfile and
ociConfigFile, respectively.)*");

REGISTER_HELP_DETAIL_TEXT(IMPORT_EXPORT_OCI_OPTIONS_DETAIL, R"*(
<b>OCI Object Storage Options</b>

@li <b>osBucketName</b>: string (default: not set) - Name of the Object Storage
bucket to use. The bucket must already exist.
@li <b>osNamespace</b>: string (default: not set) - Specifies the namespace
(tenancy name) where the bucket is located, if not given it will be obtained
using the tenancy id on the OCI configuration.
@li <b>ociConfigFile</b>: string (default: not set) - Override oci.configFile
shell option, to specify the path to the OCI configuration file.
@li <b>ociProfile</b>: string (default: not set) - Override oci.profile shell
option, to specify the name of the OCI profile to use.
)*");

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
@li <b>oci+os://region/namespace/bucket/object</b> - Read import data from
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
values: default, csv, tsv, json or csv-unix.
@li <b>decodeColumns</b>: map (default: not set) - a map between columns names
to decode methods (UNHEX or FROM_BASE64) to be applied on the loaded data.
Requires 'columns' to be set.
@li <b>characterSet</b>: string (default: not set) -
Interpret the information in the input file using this character set
encoding. characterSet set to "binary" specifies "no conversion". If not set,
the server will use the character set indicated by the character_set_database
system variable to interpret the information in the file.
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
 * Scheme part of <b>filename</b> contains infomation about the transport
 * backend. Supported transport backends are: file://, http://, https://,
 * oci+os://. If scheme part of <b>filename</b> is omitted, then file://
 * transport backend will be chosen.
 *
 * Supported filename formats:
 * @li <b>[file://]/path/to/file</b> - Read import data from local file
 * @li <b>http[s]://host.domain[:port]/path/to/file</b> - Read import data from
 * file provided in URL
 * @li <b>oci+os://region/namespace/bucket/object</b> - Read import data from
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
 * @li <b>table</b>: string (default: filename without extension) - Name of
 * target table
 * @li <b>columns</b>: string array (default: empty array) - This option takes a
 * array of column names as its value. The order of the column names indicates
 * how to match data file columns with table columns.
 * @li <b>fieldsTerminatedBy</b>: string (default: "\t"),
 * <b>fieldsEnclosedBy</b>: char (default: ''), <b>fieldsEscapedBy</b>: char
 * (default: '\\') - These options have the same meaning as the corresponding
 * clauses for LOAD DATA INFILE. For more information use <b>\\? LOAD DATA</b>,
 * (a session is required).
 * @li <b>fieldsOptionallyEnclosed</b>: bool (default: false) - Set to true if
 * the input values are not necessarily enclosed within quotation marks
 * specified by <b>fieldsEnclosedBy</b> option. Set to false if all fields are
 * quoted by character specified by <b>fieldsEnclosedBy</b> option.
 * @li <b>linesTerminatedBy</b>: string (default: "\n") - This option has the
 * same meaning as the corresponding clause for LOAD DATA INFILE. For example,
 * to import Windows files that have lines terminated with carriage
 * return/linefeed pairs, use --lines-terminated-by="\r\n". (You might have to
 * double the backslashes, depending on the escaping conventions of your command
 * interpreter.) See Section 13.2.7, "LOAD DATA INFILE Syntax".
 * @li <b>replaceDuplicates</b>: bool (default: false) - If true, input rows
 * that have the same value for a primary key or unique index as an existing row
 * will be replaced, otherwise input rows will be skipped.
 * @li <b>threads</b>: int (default: 8) - Use N threads to sent file chunks to
 * the server.
 * @li <b>bytesPerChunk</b>: string (minimum: "131072", default: "50M") - Send
 * bytesPerChunk (+ bytes to end of the row) in single LOAD DATA call. Unit
 * suffixes, k - for Kilobytes (n * 1'000 bytes), M - for Megabytes (n *
 * 1'000'000 bytes), G - for Gigabytes (n * 1'000'000'000 bytes),
 * bytesPerChunk="2k" - ~2 kilobyte data chunk will send to the MySQL Server.
 * @li <b>maxRate</b>: string (default: "0") - Limit data send throughput to
 * maxRate in bytes per second per thread.
 * maxRate="0" - no limit. Unit suffixes, k - for Kilobytes (n * 1'000 bytes),
 * M - for Megabytes (n * 1'000'000 bytes), G - for Gigabytes (n * 1'000'000'000
 * bytes), maxRate="2k" - limit to 2 kilobytes per second.
 * @li <b>showProgress</b>: bool (default: true if stdout is a tty, false
 * otherwise) - Enable or disable import progress information.
 * @li <b>skipRows</b>: int (default: 0) - Skip first n rows of the data in the
 * file. You can use this option to skip an initial header line containing
 * column names.
 * @li <b>dialect</b>: enum (default: "default") - Setup fields and lines
 * options that matches specific data file format. Can be used as base dialect
 * and customized with fieldsTerminatedBy, fieldsEnclosedBy,
 * fieldsOptionallyEnclosed, fieldsEscapedBy and linesTerminatedBy options. Must
 * be one of the following values: default, csv, tsv, json or csv-unix.
 * @li <b>decodeColumns</b>: map (default: not set) - a map between columns
 * names to decode methods (UNHEX or FROM_BASE64) to be applied on the loaded
 * data. Requires 'columns' to be set.
 * @li <b>characterSet</b>: string (default: not set) -
 * Interpret the information in the input file using this character set
 * encoding. characterSet set to "binary" specifies "no conversion". If not set,
 * the server will use the character set indicated by the character_set_database
 * system variable to interpret the information in the file.
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
 * If the <b>schema</b> is not provided, an active schema on the global session,
 * if set, will be used.
 *
 * If the input values are not necessarily enclosed within
 * <b>fieldsEnclosedBy</b>, set <b>fieldsOptionallyEnclosed</b> to true.
 *
 * If you specify one separator that is the same as or a prefix of another, LOAD
 * DATA INFILE cannot interpret the input properly.
 *
 * Connection options set in the global session, such as compression, ssl-mode,
 * etc. are used in parallel connections.
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

  auto shell_session = _shell_core.get_dev_session();
  if (!shell_session || !shell_session->is_open() ||
      shell_session->get_node_type().compare("mysql") != 0) {
    throw shcore::Exception::runtime_error(
        "A classic protocol session is required to perform this operation.");
  }

  opt.base_session(std::dynamic_pointer_cast<mysqlshdk::db::mysql::Session>(
      shell_session->get_core_session()));
  opt.validate();

  volatile bool interrupt = false;
  shcore::Interrupt_handler intr_handler([&interrupt]() -> bool {
    mysqlsh::current_console()->print_note(
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

REGISTER_HELP_FUNCTION(loadDump, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_LOADDUMP, R"*(
Loads database dumps created by MySQL Shell.

@param url URL or path to the dump directory
@param options Optional dictionary with load options

${IMPORT_EXPORT_URL_DETAIL}

<<<loadDump>>>() will load a dump from the specified path. It transparently
handles compressed files and directly streams data when loading from remote
storage (currently HTTP and OCI Object Storage). If the 'waitDumpTimeout'
option is set, it will load a dump on-the-fly, loading table data chunks as the
dumper produces them.

Table data will be loaded in parallel using the configured number of threads
(4 by default). Multiple threads per table can be used if the dump was created
with table chunking enabled. Data loads are scheduled across threads in a way
that tries to maximize parallelism, while also minimizing lock contention from
concurrent loads to the same table. If there are more tables than threads,
different tables will be loaded per thread, larger tables first. If there are
more threads than tables, then chunks from larger tables will be proportionally
assigned more threads.

LOAD DATA LOCAL INFILE is used to load table data and thus, the 'local_infile'
MySQL global setting must be enabled.

<b>Resuming</b>

The load command will store progress information into a file for each step of
the loading process, including successfully completed and interrupted/failed
ones. If that file already exists, its contents will be used to skip steps that
have already been completed and retry those that failed or didn't start yet.

When resuming, table chunks that have started being loaded but didn't finish are
loaded again. Duplicate rows are discarded by the server. Tables that do not
have unique keys are truncated before the load is resumed.

IMPORTANT: Resuming assumes that no changes have been made to the partially
loaded data between the failure and the retry. Resuming after external changes
has undefined behavior and may lead to data loss.

The progress state file has a default name of load-progress.@<server_uuid@>.json
and is written to the same location as the dump. If 'progressFile' is specified,
progress will be written to a local file at the given path. Setting it to ''
will disable progress tracking and resuming.

If the 'resetProgress' option is enabled, progress information from previous
load attempts of the dump to the destination server is discarded and the load
is restarted. You may use this option to retry loading the whole dump from the
beginning. However, changes made to the database are not reverted, so previously
loaded objects should be manually dropped first.

Options dictionary:

@li <b>analyzeTables</b>: "off", "on", "histogram" (default: off) - If 'on',
executes ANALYZE TABLE for all tables, once loaded. If set to 'histogram', only
tables that have histogram information stored in the dump will be analyzed. This
option can be used even if all 'load' options are disabled.
@li <b>characterSet</b>: string (default taken from dump) - Overrides
the character set to be used for loading dump data. By default, the same
character set used for dumping will be used (utf8mb4 if not set on dump).
@li <b>deferTableIndexes</b>: bool (default: true) - Defer all but PRIMARY index
creation for table until data has already been loaded, which should improve
performance.
@li <b>dryRun</b>: bool (default: false) - Scans the dump and prints everything
that would be performed, without actually doing so.
@li <b>excludeSchemas</b>: array of strings (default not set) - Skip loading
specified schemas from the dump.
@li <b>excludeTables</b>: array of strings (default not set) - Skip loading
specified tables from the dump. Strings are in format schema.table or
`schema`.`table`.
@li <b>ignoreExistingObjects</b>: bool (default false) - Load the dump even if
it contains objects that already exist in the target database.
@li <b>ignoreVersion</b>: bool (default false) - Load the dump even if the
major version number of the server where it was created is different from where
it will be loaded.
@li <b>includeSchemas</b>: array of strings (default not set) - Loads only the
specified schemas from the dump. By default, all schemas are included.
@li <b>includeTables</b>: array of strings (default not set) - Loads only the
specified tables from the dump. Strings are in format schema.table or
`schema`.`table`. By default, all tables from all schemas are included.
@li <b>loadData</b>: bool (default: true) - Loads table data from the dump.
@li <b>loadDdl</b>: bool (default: true) - Executes DDL/SQL scripts in the
dump.
@li <b>loadUsers</b>: bool (default: false) - Executes SQL scripts for user
accounts, roles and grants contained in the dump. Note: statements for the
current user will be skipped.
@li <b>progressFile</b>: path (default: @<server_uuid@>.progress) - Stores
load progress information in the given local file path.
@li <b>resetProgress</b>: bool (default: false) - Discards progress information
of previous load attempts to the destination server and loads the whole dump
again.
@li <b>showProgress</b>: bool (default: true if stdout is a tty, false
otherwise) - Enable or disable import progress information.
@li <b>skipBinlog</b>: bool (default: false) - Disables the binary log
for the MySQL sessions used by the loader (set sql_log_bin=0).
@li <b>threads</b>: int (default: 4) - Number of threads to use to import table
data.
@li <b>waitDumpTimeout</b>: int (default: 0) - Loads a dump while it's still
being created. Once all available tables are processed the command will either
wait for more data, the dump is marked as completed or the given timeout passes.
<= 0 disables waiting.
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS}

Connection options set in the global session, such as compression, ssl-mode, etc.
are inherited by load sessions.

Examples:

@code
util.<<<loadDump>>>("sakila_dump")

util.<<<loadDump>>>("mysql/sales", {
    "osBucketName": "mybucket",    // OCI Object Storage bucket
    "waitDumpTimeout": 1800        // wait for new data for up to 30mins
})
@endcode
)*");
/**
 * \ingroup util
 *
 * $(UTIL_LOADDUMP_BRIEF)
 *
 * $(UTIL_LOADDUMP)
 */
#if DOXYGEN_JS
Undefined loadDump(String url, Dictionary options){};
#elif DOXYGEN_PY
None load_dump(str url, dict options){};
#endif
void Util::load_dump(const std::string &url,
                     const shcore::Dictionary_t &options) {
  Load_dump_options opt(url);

  auto session = _shell_core.get_dev_session();
  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  opt.set_session(session->get_core_session());
  opt.set_options(options);
  opt.validate();

  Dump_loader loader(opt);

  shcore::Interrupt_handler intr_handler([&loader]() -> bool {
    loader.interrupt();
    return false;
  });

  auto console = mysqlsh::current_console();
  console->print_info(opt.target_import_info());

  try {
    loader.run();
  }
  CATCH_AND_TRANSLATE();
}

REGISTER_HELP_TOPIC_TEXT(TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION, R"*(
<b>MySQL Database Service Compatibility</b>

The MySQL Database Service has a few security related restrictions that
are not present in a regular, on-premise instance of MySQL. In order to make it
easier to load existing databases into the Service, the dump commands in the
MySQL Shell has options to detect potential issues and in some cases, to
automatically adjust your schema definition to be compliant.

The <b>ocimds</b> option, when set to true, will perform schema checks for
most of these issues and abort the dump if any are found. The <<<loadDump>>>
command will also only allow loading dumps that have been created with the
"ocimds" option enabled.

Some issues found by the <b>ocimds</b> option may require you to manually
make changes to your database schema before it can be loaded into the MySQL
Database Service. However, the <b>compatibility</b> option can be used to
automatically modify the dumped schema SQL scripts, resolving some of these
compatibility issues. You may pass one or more of the following options to
"compatibility", separated by a comma (,).

<b>force_innodb</b> - The MySQL Database Service requires use of the InnoDB
storage engine. This option will modify the ENGINE= clause of CREATE TABLE
statements that use incompatible storage engines and replace them with InnoDB.

<b>strip_definers</b> - strips the "DEFINER=account" clause from views, routines,
events and triggers. The MySQL Database Service requires special privileges to
create these objects with a definer other than the user loading the schema.
By stripping the DEFINER clause, these objects will be created with that default
definer. Views and Routines will additionally have their SQL SECURITY clause
changed from DEFINER to INVOKER. This ensures that the access permissions
of the account querying or calling these are applied, instead of the user that
created them. This should be sufficient for most users, but if your database
security model requires that views and routines have more privileges than their
invoker, you will need to manually modify the schema before loading it.

Please refer to the MySQL manual for details about DEFINER and SQL SECURITY.

<b>strip_restricted_grants</b> - Certain privileges are restricted in the MySQL
Database Service. Attempting to create users granting these privileges would
fail, so this option allows dumped GRANT statements to be stripped of these
privileges.

<b>strip_role_admin</b> - ROLE_ADMIN privilege can be restricted in the MySQL
Database Service, so attempting to create users granting it would fail.
This option allows dumped GRANT statements to be stripped of this privilege.

<b>strip_tablespaces</b> - Tablespaces have some restrictions in the MySQL
Database Service. If you'd like to have tables created in their default
tablespaces, this option will strip the TABLESPACE= option from CREATE TABLE
statements.

Additionally, the following changes will always be made to DDL scripts
when the <b>ocimds</b> option is enabled:

@li <b>DATA DIRECTORY</b>, <b>INDEX DIRECTORY</b> and <b>ENCRYPTION</b> options
in <b>CREATE TABLE</b> statements will be commented out.

Please refer to the MySQL Database Service documentation for more information
about restrictions and compatibility.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_COMMON_PARAMETERS_DESCRIPTION, R"*(
The <b>outputUrl</b> specifies where the dump is going to be stored.

By default, a local directory is used, and in this case <b>outputUrl</b> can be
prefixed with <b>file://</b> scheme. If a relative path is given, the absolute
path is computed as relative to the current working directory. If the output
directory does not exist but its parent does, it is created. If the output
directory exists, it must be empty. All directories created during the dump will
have the following access rights (on operating systems which support them):
<b>rwxr-x---</b>. All files created during the dump will have the following
access rights (on operating systems which support them): <b>rw-r-----</b>.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS, R"*(
@li <b>chunking</b>: bool (default: true) - Enable chunking of the tables.
@li <b>bytesPerChunk</b>: string (default: "32M") - Sets average estimated
number of bytes to be written to each chunk file, enables <b>chunking</b>.
@li <b>threads</b>: int (default: 4) - Use N threads to dump data chunks from
the server.
@li <b>maxRate</b>: string (default: "0") - Limit data read throughput to
maximum rate, measured in bytes per second per thread. Use maxRate="0" to set no
limit.
@li <b>showProgress</b>: bool (default: true if stdout is a TTY device, false
otherwise) - Enable or disable dump progress information.
@li <b>compression</b>: string (default: "zstd") - Compression used when writing
the data dump files, one of: "none", "gzip", "zstd".
@li <b>defaultCharacterSet</b>: string (default: "utf8mb4") - Character set used
for the dump.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS, R"*(
@li <b>osBucketName</b>: string (default: not set) - Use specified OCI bucket
for the location of the dump.
@li <b>osNamespace</b>: string (default: not set) - Specify the OCI namespace
(tenancy name) where the OCI bucket is located.
@li <b>ociConfigFile</b>: string (default: not set) - Use the specified OCI
configuration file instead of the one in the default location.
@li <b>ociProfile</b>: string (default: not set) - Use the specified OCI profile
instead of the default one.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS, R"*(
@li <b>events</b>: bool (default: true) - Include events from each dumped
schema.
@li <b>routines</b>: bool (default: true) - Include functions and stored
procedures for each dumped schema.
@li <b>triggers</b>: bool (default: true) - Include triggers for each dumped
table.
@li <b>tzUtc</b>: bool (default: true) - Convert TMESTAMP data to UTC.
@li <b>consistent</b>: bool (default: true) - Enable or disable consistent data
dumps.
@li <b>ddlOnly</b>: bool (default: false) - Only dump Data Definition Language
(DDL) from the database.
@li <b>dataOnly</b>: bool (default: false) - Only dump data from the database.
@li <b>dryRun</b>: bool (default: false) - Print information about what would be
dumped, but do not dump anything.
@li <b>ocimds</b>: bool (default: false) - Enable checks for compatibility with
MySQL Database Service (MDS)
@li <b>compatibility</b>: list of strings (default: empty) - Apply MySQL
Database Service compatibility modifications when writing dump files. Supported
values: "force_innodb", "strip_definers", "strip_restricted_grants",
"strip_role_admin", "strip_tablespaces".
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_COMMON_OPTIONS, R"*(
@li <b>excludeTables</b>: list of strings (default: empty) - List of tables to
be excluded from the dump in the format of <b>schema</b>.<b>table</b>.
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_COMMON_DETAILS, R"*(
<b>Requirements</b>
@li MySQL Server 5.7 or newer is required.
@li Schema object names must use latin1 or utf8 character set.
@li Only tables which use the InnoDB storage engine are guaranteed to be dumped
with consistent data.
@li File size limit for files uploaded to the OCI bucket is 1.2 TiB.

<b>Details</b>

This operation writes SQL files per each schema, table and view dumped, along
with some global SQL files.

Table data dumps are written to TSV files, optionally splitting them into
multiple chunk files.

Requires an open, global Shell session, and uses its connection
options, such as compression, ssl-mode, etc., to establish additional
connections.

Data dumps cannot be created for the following tables:
@li mysql.apply_status
@li mysql.general_log
@li mysql.schema
@li mysql.slow_log
)*");

REGISTER_HELP_DETAIL_TEXT(TOPIC_UTIL_DUMP_COMMON_OPTION_DETAILS, R"*(
The names given in the <b>excludeTables</b> option should be valid MySQL
identifiers, quoted using backtick characters when required.

If the <b>excludeTables</b> option contains a table which does not exist, or a
table which belongs to a schema which is not included in the dump or does not
exist, it is ignored.

The <b>tzUtc</b> option allows dumping TIMESTAMP data when a server has data in
different time zones or data is being moved between servers with different time
zones.

If the <b>consistent</b> option is set to true, a global read lock is set using
the <b>FLUSH TABLES WITH READ LOCK</b> statement, all threads establish
connections with the server and start transactions using:
@li SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ
@li START TRANSACTION WITH CONSISTENT SNAPSHOT

Once all the threads start transactions, the instance is locked for backup and
the global read lock is released.

The <b>ddlOnly</b> and <b>dataOnly</b> options cannot both be set to true at
the same time.

The <b>chunking</b> option causes the the data from each table to be split and
written to multiple chunk files. If this option is set to false, table data is
written to a single file.

If the <b>chunking</b> option is set to <b>true</b>, but a table to be dumped
cannot be chunked (for example if it does not contain a primary key or a unique
index), a warning is displayed and chunking is disabled for this table.

The value of the <b>threads</b> option must be a positive number.

Both the <b>bytesPerChunk</b> and <b>maxRate</b> options support unit suffixes:
@li k - for Kilobytes (n * 1'000 bytes),
@li M - for Megabytes (n * 1'000'000 bytes),
@li G - for Gigabytes (n * 1'000'000'000 bytes),

i.e. maxRate="2k" - limit throughput to 2 kilobytes per second.

The value of the <b>bytesPerChunk</b> option cannot be smaller than "128k".

${TOPIC_UTIL_DUMP_COMPATIBILITY_OPTION}

<b>Dumping to a Bucket in the OCI Object Storage</b>

If the <b>osBucketName</b> option is used, the dump is stored in the specified
OCI bucket, connection is established using the local OCI profile. The directory
structure is simulated within the object name.

The <b>osNamespace</b>, <b>ociConfigFile</b> and <b>ociProfile</b> options
cannot be used if option <b>osBucketName</b> is set to an empty string.

The <b>osNamespace</b> option overrides the OCI namespace obtained based on the
tenancy ID from the local OCI profile.
)*");

REGISTER_HELP_FUNCTION(dumpSchemas, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPSCHEMAS, R"*(
Dumps the specified schemas to the files in the output directory.

@param schemas List of schemas to be dumped.
@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

The <b>schemas</b> parameter cannot be an empty list.

${TOPIC_UTIL_DUMP_COMMON_PARAMETERS_DESCRIPTION}

<b>The following options are supported:</b>
${TOPIC_UTIL_DUMP_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS}

${TOPIC_UTIL_DUMP_COMMON_DETAILS}

<b>Options</b>

${TOPIC_UTIL_DUMP_COMMON_OPTION_DETAILS}

@throws ArgumentError in the following scenarios:
@li If any of the input arguments contains an invalid value.

@throws RuntimeError in the following scenarios:
@li If there is no open global session.
@li If creating the output directory fails.
@li If creating or writing to the output file fails.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPSCHEMAS_BRIEF)
 *
 * $(UTIL_DUMPSCHEMAS)
 */
#if DOXYGEN_JS
Undefined Util::dumpSchemas(List schemas, String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::dump_schemas(list schemas, str outputUrl, dict options);
#endif
void Util::dump_schemas(const std::vector<std::string> &schemas,
                        const std::string &directory,
                        const shcore::Dictionary_t &options) {
  const auto session = _shell_core.get_dev_session();

  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  using mysqlsh::dump::Dump_schemas;
  using mysqlsh::dump::Dump_schemas_options;

  Dump_schemas_options opts{schemas, directory};
  opts.set_options(options);
  opts.set_session(session->get_core_session());

  Dump_schemas{opts}.run();
}

REGISTER_HELP_FUNCTION(dumpInstance, util);
REGISTER_HELP_FUNCTION_TEXT(UTIL_DUMPINSTANCE, R"*(
Dumps the whole database to files in the output directory.

@param outputUrl Target directory to store the dump files.
@param options Optional dictionary with the dump options.

${TOPIC_UTIL_DUMP_COMMON_PARAMETERS_DESCRIPTION}

<b>The following options are supported:</b>
@li <b>excludeSchemas</b>: list of strings (default: empty) - list of schemas to
be excluded from the dump.
${TOPIC_UTIL_DUMP_COMMON_OPTIONS}
@li <b>users</b>: bool (default: true) - Include users, roles and grants in the
dump file.
${TOPIC_UTIL_DUMP_DDL_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_EXPORT_COMMON_OPTIONS}
${TOPIC_UTIL_DUMP_OCI_COMMON_OPTIONS}

${TOPIC_UTIL_DUMP_COMMON_DETAILS}

Dumps cannot be created for the following schemas:
@li information_schema,
@li mysql,
@li ndbinfo,
@li performance_schema,
@li sys.

<b>Options</b>

If the <b>excludeSchemas</b> option contains a schema which is not included in
the dump or does not exist, it is ignored.

${TOPIC_UTIL_DUMP_COMMON_OPTION_DETAILS}

@throws ArgumentError in the following scenarios:
@li If any of the input arguments contains an invalid value.

@throws RuntimeError in the following scenarios:
@li If there is no open global session.
@li If creating the output directory fails.
@li If creating or writing to the output file fails.
)*");

/**
 * \ingroup util
 *
 * $(UTIL_DUMPINSTANCE_BRIEF)
 *
 * $(UTIL_DUMPINSTANCE)
 */
#if DOXYGEN_JS
Undefined Util::dumpInstance(String outputUrl, Dictionary options);
#elif DOXYGEN_PY
None Util::dump_instance(str outputUrl, dict options);
#endif
void Util::dump_instance(const std::string &directory,
                         const shcore::Dictionary_t &options) {
  const auto session = _shell_core.get_dev_session();

  if (!session || !session->is_open()) {
    throw std::runtime_error(
        "An open session is required to perform this operation.");
  }

  using mysqlsh::dump::Dump_instance;
  using mysqlsh::dump::Dump_instance_options;

  Dump_instance_options opts{directory};
  opts.set_options(options);
  opts.set_session(session->get_core_session());

  Dump_instance{opts}.run();
}

}  // namespace mysqlsh

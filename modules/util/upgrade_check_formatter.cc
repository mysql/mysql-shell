/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_check_formatter.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <sstream>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {

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

}  // namespace

class Text_upgrade_checker_output : public Upgrade_check_output_formatter {
 public:
  Text_upgrade_checker_output() : m_console(mysqlsh::current_console()) {}

  void check_info(const std::string &server_addres,
                  const std::string &server_version,
                  const std::string &target_version) override {
    print_paragraph(
        shcore::str_format(
            "The MySQL server at %s, version %s, will now be checked for "
            "compatibility issues for upgrade to MySQL %s...",
            server_addres.c_str(), server_version.c_str(),
            target_version.c_str()),
        0, 0);
  }

  void check_results(const Upgrade_check &check,
                     const std::vector<Upgrade_issue> &results) override {
    print_title(check.get_title());

    std::function<std::string(const Upgrade_issue &)> issue_formater(
        upgrade_issue_to_string);
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

    if (error > 0) {
      m_console->print_error(text);
    } else if (warning > 0 || notice > 0) {
      m_console->print_note(text);
    } else {
      m_console->println(text);
    }
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
  if (shcore::str_caseeq(format, "JSON"))
    return std::make_unique<JSON_upgrade_checker_output>();
  else if (shcore::str_caseeq(format, "TEXT"))
    return std::make_unique<Text_upgrade_checker_output>();

  throw std::invalid_argument(
      "Allowed values for outputFormat parameter are TEXT or JSON");
}

}  // namespace mysqlsh

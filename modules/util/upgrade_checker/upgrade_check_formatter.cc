/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/util/upgrade_checker/upgrade_check_formatter.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <sstream>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/upgrade_checker/feature_life_cycle_check.h"
#include "modules/util/upgrade_checker/upgrade_check_condition.h"

namespace mysqlsh {
namespace upgrade_checker {

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

std::string multi_lvl_format_issue(const Upgrade_issue &problem) {
  return shcore::str_format("%s: %s",
                            Upgrade_issue::level_to_string(problem.level),
                            upgrade_issue_to_string(problem).c_str());
}

}  // namespace

class Text_upgrade_checker_output : public Upgrade_check_output_formatter {
 public:
  Text_upgrade_checker_output() : m_console(mysqlsh::current_console()) {}

  void check_info(const std::string &server_address,
                  const std::string &server_version,
                  const std::string &target_version,
                  bool explicit_target_version,
                  const std::string &warning) override {
    print_paragraph(
        shcore::str_format(
            "The MySQL server at %s, version %s, will now be checked for "
            "compatibility issues for upgrade to MySQL %s%s.",
            server_address.c_str(), server_version.c_str(),
            target_version.c_str(),
            explicit_target_version
                ? ""
                : ". To check for a different target server version, use the "
                  "targetVersion option"),
        0, 0);

    if (!warning.empty()) {
      m_console->println();
      print_paragraph("WARNING: " + warning, 0, 0);
    }
  }

  void check_title(const Upgrade_check &check) override {
    print_title(check.get_title(), check.get_name());
  }

  void check_results(const Upgrade_check &check,
                     const std::vector<Upgrade_issue> &results) override {
    std::function<std::string(const Upgrade_issue &)> issue_formater(
        upgrade_issue_to_string);
    if (results.empty()) {
      print_paragraph("No issues found");
    } else {
      if (!check.groups().empty()) {
        print_grouped_issues(check, results);
      } else {
        if (!check.get_description().empty()) {
          print_paragraph(check.get_description());
          print_doc_links(check.get_doc_link());
          m_console->println();

          if (check.is_multi_lvl_check())
            issue_formater = multi_lvl_format_issue;
        } else {
          issue_formater = format_upgrade_issue;
        }

        for (const auto &issue : results) {
          print_paragraph(issue_formater(issue));
          print_doc_links(issue.doclink);
        }
      }
    }
  }

  void check_error(const Upgrade_check &check, const char *description,
                   bool runtime_error = true) override {
    m_console->print("  ");
    if (runtime_error) m_console->print_diag("Check failed: ");
    m_console->println(description);
    print_doc_links(check.get_doc_link());
  }

  void manual_check(const Upgrade_check &check) override {
    print_title(check.get_title(), check.get_name());
    print_paragraph(check.get_description());
    print_doc_links(check.get_doc_link());
  }

  void summarize(int error, int warning, int notice, const std::string &text,
                 const std::map<std::string, std::string> &excluded) override {
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

    if (!excluded.empty()) {
      m_console->println();
      m_console->println(
          "WARNING: The following checks were excluded (per user request):");
      for (const auto &item : excluded) {
        m_console->println(shcore::str_format("%s: %s", item.first.c_str(),
                                              item.second.c_str()));
      }
    }
  }

  void list_info(const std::string &server_address,
                 const std::string &server_version,
                 const std::string &target_version,
                 bool explicit_target_version) override {
    print_paragraph("Upgrade Consistency Checks");
    m_console->println();
    if (!server_address.empty() && !server_version.empty() &&
        !target_version.empty()) {
      print_paragraph(
          shcore::str_format(
              "The following compatibility checks will be performed for an "
              "upgrade of MySQL Server at %s, version %s to %s%s:",
              server_address.c_str(), server_version.c_str(),
              target_version.c_str(),
              explicit_target_version ? ""
                                      : ". To list checks for a different "
                                        "target server version, use the "
                                        "targetVersion option"),
          0, 0);
    } else {
      print_paragraph(
          "The MySQL Shell will now list checks for possible compatibility "
          "issues for upgrade of MySQL Server...");
    }
  }

  void list_item_infos(
      const std::string &section,
      const std::vector<std::unique_ptr<Upgrade_check>> &checks) override {
    m_console->println();
    m_console->println(shcore::str_format("%s:", section.c_str()));

    if (checks.empty()) {
      print_paragraph("Empty.");
    } else {
      for (const auto &check : checks) {
        list_item_info(*check);
      }
    }
  }

  void list_summarize(size_t included, size_t excluded) override {
    m_console->println();
    if (included > 0)
      m_console->println(shcore::str_format("Included: %zu", included));
    if (excluded > 0)
      m_console->println(shcore::str_format("Excluded: %zu", excluded));
    if (included > 0 || excluded > 0) m_console->println();
  }

 private:
  void list_item_info(const Upgrade_check &check) {
    m_console->println();
    print_paragraph("- " + check.get_name(), 0);
    print_paragraph(check.get_title());
    if (check.get_condition())
      print_paragraph("Condition: " + check.get_condition()->description());
  }

  void print_title(const std::string &title, const std::string &name) {
    m_console->println();
    print_paragraph(shcore::str_format("%d) %s (%s)", ++m_check_count,
                                       title.c_str(), name.c_str()),
                    0, 0);
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

  void print_doc_links(const std::string &links) {
    if (!links.empty()) {
      std::string indent(2, ' ');
      m_console->println(indent + "More information:");
      print_paragraph(links, 4, 0);
    }
  }

  void print_grouped_issues(const Upgrade_check &check,
                            const std::vector<Upgrade_issue> &results) {
    std::map<std::string, std::set<std::string>> grouped_issues;
    std::map<std::string, Upgrade_issue::Level> group_levels;

    for (const auto &issue : results) {
      auto issue_desc = issue.get_db_object();
      if (!issue.description.empty()) issue_desc += ": " + issue.description;
      grouped_issues[issue.group].insert(issue_desc);
      group_levels[issue.group] = issue.level;
    }

    for (const auto &group : check.groups()) {
      auto issues = grouped_issues.find(group);

      // No issues on this group
      if (issues == grouped_issues.end()) {
        continue;
      }

      // Prints the group description first
      auto issue_level = Upgrade_issue::level_to_string(group_levels[group]);
      print_paragraph(shcore::str_format("%s: %s", issue_level,
                                         check.get_description(group).c_str()),
                      2, 0);

      for (const auto &item : issues->second) {
        print_paragraph("- " + item, 2, 0);
      }
      m_console->println();

      auto doc_links = check.get_doc_link(group);
      if (!doc_links.empty()) {
        print_doc_links(doc_links);
        m_console->println();
      }
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
                  const std::string &target_version,
                  [[maybe_unused]] bool explicit_target_version,
                  [[maybe_unused]] const std::string &warning) override {
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

  void check_title(const Upgrade_check &) override {}

  void check_results(const Upgrade_check &check,
                     const std::vector<Upgrade_issue> &results) override {
    rapidjson::Value check_object(rapidjson::kObjectType);
    rapidjson::Value id;
    check_object.AddMember("id", rapidjson::StringRef(check.get_name().c_str()),
                           m_allocator);
    check_object.AddMember(
        "title", rapidjson::StringRef(check.get_title().c_str()), m_allocator);
    check_object.AddMember("status", rapidjson::StringRef("OK"), m_allocator);
    if (!results.empty()) {
      rapidjson::Value description;
      auto check_description = check.get_description();
      if (!check_description.empty())
        description.SetString(check_description.c_str(),
                              check_description.length(), m_allocator);
      check_object.AddMember("description", description, m_allocator);
      if (!check.get_doc_link().empty())
        check_object.AddMember(
            "documentationLink",
            rapidjson::StringRef(check.get_doc_link().c_str()), m_allocator);
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
      // If it is a grouped issue with no specific issue description, uses the
      // group description
      if (!issue.group.empty() && issue.description.empty()) {
        auto group_description = check.get_description(issue.group);
        description.SetString(group_description.c_str(),
                              group_description.length(), m_allocator);
      } else {
        description.SetString(issue.description.c_str(),
                              issue.description.length(), m_allocator);
      }

      issue_object.AddMember("description", description, m_allocator);

      if (!issue.doclink.empty()) {
        rapidjson::Value doclink;
        doclink.SetString(issue.doclink.c_str(), issue.doclink.length(),
                          m_allocator);
        issue_object.AddMember("documentationLink", doclink, m_allocator);
      }

      auto type = issue.type_to_string(issue.object_type);
      rapidjson::Value object_type;
      object_type.SetString(type.data(), type.length(), m_allocator);
      issue_object.AddMember("dbObjectType", object_type, m_allocator);

      issues.PushBack(issue_object, m_allocator);
    }

    check_object.AddMember("detectedProblems", issues, m_allocator);
    m_checks.PushBack(check_object, m_allocator);
  }

  void check_error(const Upgrade_check &check, const char *description,
                   bool runtime_error = true) override {
    rapidjson::Value check_object(rapidjson::kObjectType);

    check_object.AddMember("id", rapidjson::StringRef(check.get_name().c_str()),
                           m_allocator);
    check_object.AddMember(
        "title", rapidjson::StringRef(check.get_title().c_str()), m_allocator);
    if (runtime_error)
      check_object.AddMember("status", rapidjson::StringRef("ERROR"),
                             m_allocator);
    else
      check_object.AddMember(
          "status", rapidjson::StringRef("CONFIGURATION_ERROR"), m_allocator);

    rapidjson::Value descr;
    descr.SetString(description, strlen(description), m_allocator);
    check_object.AddMember("description", descr, m_allocator);
    if (!check.get_doc_link().empty())
      check_object.AddMember("documentationLink",
                             rapidjson::StringRef(check.get_doc_link().c_str()),
                             m_allocator);

    rapidjson::Value issues(rapidjson::kArrayType);
    m_checks.PushBack(check_object, m_allocator);
  }

  void manual_check(const Upgrade_check &check) override {
    rapidjson::Value check_object(rapidjson::kObjectType);
    rapidjson::Value id;
    check_object.AddMember("id", rapidjson::StringRef(check.get_name().c_str()),
                           m_allocator);
    check_object.AddMember(
        "title", rapidjson::StringRef(check.get_title().c_str()), m_allocator);

    auto description = check.get_description();
    if (!description.empty()) {
      rapidjson::Value descr;
      descr.SetString(description.c_str(), description.length(), m_allocator);
      check_object.AddMember("description", descr, m_allocator);
    }
    if (!check.get_doc_link().empty())
      check_object.AddMember("documentationLink",
                             rapidjson::StringRef(check.get_doc_link().c_str()),
                             m_allocator);
    m_manual_checks.PushBack(check_object, m_allocator);
  }

  void summarize(int error, int warning, int notice, const std::string &text,
                 const std::map<std::string, std::string> &excluded) override {
    m_json_document.AddMember("errorCount", error, m_allocator);
    m_json_document.AddMember("warningCount", warning, m_allocator);
    m_json_document.AddMember("noticeCount", notice, m_allocator);

    rapidjson::Value val;
    val.SetString(text.c_str(), text.length(), m_allocator);
    m_json_document.AddMember("summary", val, m_allocator);

    m_json_document.AddMember("checksPerformed", m_checks, m_allocator);
    m_json_document.AddMember("manualChecks", m_manual_checks, m_allocator);

    if (!excluded.empty()) {
      rapidjson::Value excluded_list(rapidjson::kArrayType);

      for (const auto &item : excluded) {
        rapidjson::Value exclude_item(rapidjson::kObjectType);

        exclude_item.AddMember("id", rapidjson::StringRef(item.first.c_str()),
                               m_allocator);

        exclude_item.AddMember(
            "title", rapidjson::StringRef(item.second.c_str()), m_allocator);

        excluded_list.PushBack(exclude_item, m_allocator);
      }

      m_json_document.AddMember("excludedChecks", excluded_list, m_allocator);
    }

    print_to_output();
  }

  void list_info(const std::string &server_address,
                 const std::string &server_version,
                 const std::string &target_version,
                 [[maybe_unused]] bool explicit_target_version) override {
    if (!server_address.empty()) {
      rapidjson::Value addr;
      addr.SetString(server_address.c_str(), server_address.length(),
                     m_allocator);
      m_json_document.AddMember("serverAddress", addr, m_allocator);
    }

    if (!server_version.empty()) {
      rapidjson::Value svr;
      svr.SetString(server_version.c_str(), server_version.length(),
                    m_allocator);
      m_json_document.AddMember("serverVersion", svr, m_allocator);
    }

    if (!target_version.empty()) {
      rapidjson::Value tvr;
      tvr.SetString(target_version.c_str(), target_version.length(),
                    m_allocator);
      m_json_document.AddMember("targetVersion", tvr, m_allocator);
    }
  }

  void list_item_infos(
      const std::string &section,
      const std::vector<std::unique_ptr<Upgrade_check>> &checks) override {
    rapidjson::Value checks_array(rapidjson::kArrayType);

    for (const auto &check : checks) {
      rapidjson::Value info_object(rapidjson::kObjectType);
      info_object.AddMember(
          "id", rapidjson::StringRef(check->get_name().c_str()), m_allocator);
      info_object.AddMember("title",
                            rapidjson::StringRef(check->get_title().c_str()),
                            m_allocator);
      if (check->get_condition()) {
        rapidjson::Value val;
        auto text = check->get_condition()->description();
        val.SetString(text.c_str(), text.length(), m_allocator);
        info_object.AddMember("condition", val, m_allocator);
      }
      checks_array.PushBack(info_object, m_allocator);
    }
    auto lower_section = shcore::str_lower(section);
    rapidjson::Value val;
    val.SetString(lower_section.c_str(), lower_section.length(), m_allocator);
    m_json_document.AddMember(val, checks_array, m_allocator);
  }

  void list_summarize(size_t included, size_t excluded) override {
    m_json_document.AddMember("included", static_cast<uint64_t>(included),
                              m_allocator);
    m_json_document.AddMember("excluded", static_cast<uint64_t>(excluded),
                              m_allocator);
    print_to_output();
  }

  const rapidjson::Document &get_document() const { return m_json_document; }

 private:
  void print_to_output() {
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

}  // namespace upgrade_checker
}  // namespace mysqlsh

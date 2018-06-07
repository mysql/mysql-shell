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

#include "src/mysqlsh/commands/command_help.h"

#include <algorithm>
#include <map>
#include <set>

#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/include/shellcore/base_session.h"

namespace textui = mysqlshdk::textui;
using shcore::Topic_mask;
using shcore::Topic_type;

namespace mysqlsh {
bool Command_help::execute(const std::vector<std::string> &args) {
  // Gets the entire help system
  auto help = _shell->get_helper();

  // If help came with parameter attempts to print the
  // specific help on the active shell first and global commands
  if (args.size() > 1) {
    // Returned topics must be remain valid for the printing functions
    // So this needs to be declated here.
    std::vector<shcore::Help_topic> sql_topics;
    auto tokens = shcore::str_split(args[0], " ", 1);
    auto pattern = tokens[1];
    std::vector<shcore::Help_topic *> topics;
    bool is_sql_search = false;

    // Avoids local search if it is for sure a SQL search
    is_sql_search = shcore::str_ibeginswith(pattern.c_str(),
                                            shcore::Help_registry::HELP_SQL);

    if (!_shell->get_dev_session() && is_sql_search) {
      _console->println(
          "SQL help requires the Shell to be connected to a "
          "MySQL server.\n");
    } else {
      // On an SQL seach, avoids looking at the shell help
      if (!is_sql_search) topics = help->search_topics(pattern);

      // We also avoid the search if the 'help contents' was requested
      // because that will return the main categories for MySQL help
      // and the shell help contents is different
      if (topics.empty() ||
          (*topics.begin())->m_name != shcore::Help_registry::HELP_ROOT) {
        sql_topics = get_sql_topics(pattern);
        for (auto &sql_topic : sql_topics) {
          topics.push_back(&sql_topic);
        }
      }
      if (topics.empty()) {
        _console->println("No help items found matching " +
                          textui::bold(pattern));
      } else if (topics.size() == 1) {
        auto topic = *topics.begin();
        if (is_sql_search || topic->is_sql()) {
          _shell->print((*topics.begin())->m_help_tag);
        } else {
          auto data = _shell->get_helper()->get_help(**topics.begin());

          if (!data.empty()) {
            _console->println(data);
          }
        }
      } else {
        print_help_multiple_topics(pattern, topics);
      }
    }
  } else {
    print_help_global();
  }

  return true;
}

std::vector<shcore::Help_topic> Command_help::get_sql_topics(
    const std::string &pattern) {
  std::vector<shcore::Help_topic> sql_topics;
  shcore::Help_topic *sql;
  std::string sql_pattern(pattern);

  auto dev_session = _shell->get_dev_session();
  if (dev_session) {
    if (shcore::str_ibeginswith(sql_pattern.c_str(), "SQL Syntax")) {
      sql_pattern = pattern.substr(10);

      if (sql_pattern.empty() || sql_pattern == "/")
        sql_pattern = "contents";
      else if (sql_pattern[0] == '/')
        sql_pattern = sql_pattern.substr(1);
    }

    // Converts the pattern from glob to sql
    sql_pattern = glob_to_sql(sql_pattern, "*", "%");
    sql_pattern = glob_to_sql(sql_pattern, "?", "_");

    sql = *_shell->get_helper()
               ->search_topics(shcore::Help_registry::HELP_SQL)
               .begin();

    auto session = dev_session->get_core_session();
    if (session && session->is_open()) {
      auto result = session->queryf("help ?", sql_pattern);
      if (result->has_resultset()) {
        auto metadata = result->get_metadata();

        // Three possible results:
        // - Multiple Topics
        //   +------+----------------+
        //   | name | is_it_category |
        //   +------+----------------+
        // - Multiple categories
        //   +----------------------+------+----------------+
        //   | source_category_name | name | is_it_category |
        //   +----------------------+------+----------------+
        // - Single Topic
        //   +------+-------------+
        //   | name | description |
        //   +------+-------------+
        size_t name_col = 0;
        if (shcore::str_casecmp(metadata[0].get_column_label(), "name") != 0)
          name_col++;

        size_t desc_col = 0;
        if (shcore::str_casecmp(metadata[1].get_column_label(),
                                "description") == 0)
          desc_col++;

        // If found a list of topics
        if (desc_col == 0) {
          auto entry = result->fetch_one();
          while (entry) {
            std::string name = entry->get_string(name_col);
            sql_topics.push_back({sql->m_id + "/" + name,
                                  name,
                                  shcore::Topic_type::SQL,
                                  name,
                                  sql,
                                  {}});
            entry = result->fetch_one();
          }
          // If found a specific topi we are overloading the tag field to
          // hold the actual help description
        } else {
          auto entry = result->fetch_one();
          std::string name = entry->get_string(name_col);
          sql_topics.push_back({sql->m_id + "/" + name,
                                entry->get_string(0),
                                shcore::Topic_type::SQL,
                                entry->get_string(desc_col),
                                sql,
                                {}});
        }
      }
    }
  }

  return sql_topics;
}

struct Id_compare {
  bool operator()(shcore::Help_topic *const &lhs,
                  shcore::Help_topic *const &rhs) const {
    if (!lhs) {
      return true;
    } else if (!rhs) {
      return false;
    } else {
      int ret_val = shcore::str_casecmp(lhs->m_id.c_str(), rhs->m_id.c_str());

      // If case insensitive are equal, does case sensitive comparison
      return ret_val == 0 ? (lhs->m_id < rhs->m_id) : ret_val < 0;
    }
  }
};

void Command_help::print_help_multiple_topics(
    const std::string &pattern,
    const std::vector<shcore::Help_topic *> &topics) {
  std::vector<std::string> output;
  output.push_back("Found several entries matching <b>" + pattern + "</b>");

  std::map<std::string, std::set<shcore::Help_topic *, Id_compare>> groups;
  for (auto topic : topics) {
    if (groups.find(topic->get_category()->m_name) == groups.end())
      groups[topic->get_category()->m_name] = {};

    groups[topic->get_category()->m_name].insert(topic);
  }

  for (auto group : groups) {
    output.push_back("The following topics were found at the <b>" +
                     group.first + "</b> category:");

    for (auto topic : group.second) {
      output.push_back("@li " + topic->get_id(groups.size() > 1,
                                              _shell->interactive_mode()));
    }
  }

  auto topic = *groups.begin()->second.begin();
  output.push_back("For help on a specific topic use: <b>\\?</b> <topic>");
  output.push_back(
      "e.g.: <b>\\?</b> " +
      topic->get_id(groups.size() > 1, _shell->interactive_mode()));

  _console->println(textui::format_markup_text(output, 80, 0));
}

void Command_help::print_help_global() {
  std::vector<std::string> sections;

  auto help = _shell->get_helper();

  sections.push_back(help->get_help(shcore::Help_registry::HELP_ROOT,
                                    Topic_mask(), "detail,categories"));

  if (_shell->interactive_mode() == shcore::IShell_core::Mode::SQL) {
    help->add_section("", "HELP_AVAILABLE_TOPICS_SQL", &sections, 0);
  } else {
    help->add_section("", "HELP_AVAILABLE_TOPICS_SCRIPTING", &sections, 0);
  }

  sections.push_back(textui::bold("SHELL COMMANDS"));
  sections.push_back(help->get_help(shcore::Help_registry::HELP_COMMANDS,
                                    Topic_mask(), "detail,childs"));

  // Retrieves the global objects if in a scripting mode
  if (_shell->interactive_mode() != shcore::IShell_core::Mode::SQL) {
    std::vector<shcore::Help_topic> global_topics;
    std::vector<shcore::Help_topic *> global_refs;
    // shcore::Help_topic_refs global_refs;
    auto globals = _shell->get_global_objects(_shell->interactive_mode());

    if (globals.size()) {
      for (auto name : globals) {
        auto object_val = _shell->get_global(name);
        auto object = std::dynamic_pointer_cast<shcore::Cpp_object_bridge>(
            object_val.as_object());
        global_topics.push_back({"",
                                 name,
                                 shcore::Topic_type::CLASS,
                                 object->class_name() + "_GLOBAL",
                                 nullptr,
                                 {},
                                 nullptr});
      }
    }

    // Inserts the default modules
    global_topics.push_back({"",
                             "mysqlx",
                             shcore::Topic_type::MODULE,
                             "MYSQLX_GLOBAL",
                             nullptr,
                             {},
                             nullptr});
    global_topics.push_back({"",
                             "mysql",
                             shcore::Topic_type::TOPIC,
                             "MYSQL_GLOBAL",
                             nullptr,
                             {},
                             nullptr});

    // We need the references to use the existing formatting code
    for (auto &topic : global_topics) global_refs.push_back(&topic);

    std::sort(global_refs.begin(), global_refs.end(),
              shcore::Help_topic_compare());

    sections.push_back(textui::bold("GLOBAL OBJEECTS"));
    _shell->get_helper()->add_childs_section(global_refs, &sections, 0, false,
                                             "GLOBALS", "GLOBALS");
    help->add_examples_section("GLOBALS_EXAMPLE_SCRIPTING", &sections, 0);
  } else {
    help->add_examples_section("GLOBALS_EXAMPLE_SQL", &sections, 0);
  }

  _console->println(shcore::str_join(sections, "\n\n"));
}

std::string Command_help::glob_to_sql(const std::string &pattern,
                                      const std::string &glob,
                                      const std::string &sql) {
  std::string sql_pattern(pattern);

  // Preexisting sql pattern were literal in glob, so they are escaped for sql
  sql_pattern = shcore::str_replace(sql_pattern, sql, "\\" + sql);

  // Escaped glob pattern must be backed up, to be unescaped for sql
  sql_pattern = shcore::str_replace(sql_pattern, "\\" + pattern, "#$#");

  // pattern replacements
  sql_pattern = shcore::str_replace(sql_pattern, glob, sql);

  // Restore the backups, now unescaped
  sql_pattern = shcore::str_replace(sql_pattern, "#$#", pattern);

  return sql_pattern;
}

}  // namespace mysqlsh

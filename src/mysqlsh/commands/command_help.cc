/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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
#include <optional>
#include <set>
#include "mysqlshdk/include/shellcore/base_session.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/textui/textui.h"

namespace textui = mysqlshdk::textui;
using shcore::Topic_mask;
using shcore::Topic_type;

namespace mysqlsh {
namespace {
/**
 * Traverses the topic list trying to identify a unique match given the received
 * matcher
 *
 * @param pattern The pattern used to query for help
 * @param topics The topics found with the given pattern.
 * @param matcher flag to identify the case to be used on the comparison
 * @returns the index of the unique match found if any, otherwise an empty
 * index.
 */
std::optional<int> find_unique_match(
    const std::vector<const shcore::Help_topic *> &topics,
    const std::function<bool(const shcore::Help_topic *)> &matcher) {
  // Verifies if in the topics there is an exact match case sensitive;
  std::optional<int> match_index;

  for (size_t index = 0; index < topics.size(); index++) {
    if (matcher(topics[index])) {
      if (!match_index.has_value()) {
        // At the first found the index is set
        match_index = index;
      } else {
        // If more than one found the index is reset and we leave as there's no
        // unique
        match_index.reset();
        break;
      }
    }
  }

  return match_index;
}
}  // namespace
bool Command_help::execute(const std::vector<std::string> &args) {
  // Gets the entire help system
  auto help = _shell->get_helper();

  // If help came with parameter attempts to print the
  // specific help on the active shell first and global commands
  if (args.size() > 1) {
    // Returned topics must be remain valid for the printing functions
    // So this needs to be declated here.
    std::vector<shcore::Help_topic> sql_topics;
    const auto tokens = shcore::str_split(shcore::str_strip(args[0]), " ", 1);
    const auto pattern = shcore::str_strip(tokens[1]);
    std::vector<const shcore::Help_topic *> topics;
    bool is_sql_search = false;

    // Avoids local search if it is for sure a SQL search
    is_sql_search = shcore::str_ibeginswith(pattern.c_str(),
                                            shcore::Help_registry::HELP_SQL);

    auto console = mysqlsh::current_console();

    if (is_sql_search &&
        (!_shell->get_dev_session() || !_shell->get_dev_session()->is_open())) {
      console->println(
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
        console->println("No help items found matching " +
                         textui::remark(pattern));
      } else if (topics.size() == 1) {
        auto topic = *topics.begin();
        if (is_sql_search || topic->is_sql()) {
          console->print((*topics.begin())->m_help_tag);
        } else {
          auto data = _shell->get_helper()->get_help(**topics.begin());

          if (!data.empty()) {
            console->println(data);
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
  const shcore::Help_topic *sql;
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
                                  const_cast<shcore::Help_topic *>(sql),
                                  {},
                                  true});
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
                                const_cast<shcore::Help_topic *>(sql),
                                {},
                                true});
        }
      }
    }
  }

  return sql_topics;
}

void Command_help::print_help_multiple_topics(
    const std::string &pattern,
    const std::vector<const shcore::Help_topic *> &topics) {
  std::map<std::string,
           std::set<const shcore::Help_topic *, shcore::Help_topic_id_compare>>
      groups;

  // Let's see if from the list, any topic fully qualified id matches the
  // searched string
  std::optional<int> index;
  auto mode = _shell->get_helper()->get_mode();
  index = find_unique_match(topics,
                            [&pattern, mode](const shcore::Help_topic *topic) {
                              return topic->get_id(mode) == pattern;
                            });

  // If no matches so far, tries a case sensitive exact match on the found
  // topics
  if (!index.has_value()) {
    index =
        find_unique_match(topics, [&pattern](const shcore::Help_topic *topic) {
          return topic->m_name == pattern;
        });
  }

  // If no matches so far, tries a case sensitive exact match on the found
  // topics
  if (!index.has_value()) {
    index =
        find_unique_match(topics, [&pattern](const shcore::Help_topic *topic) {
          return shcore::str_caseeq(pattern, topic->m_name);
        });
  }

  // Finally it tries a case search by name and global objects
  if (!index.has_value()) {
    index =
        find_unique_match(topics, [&pattern](const shcore::Help_topic *topic) {
          return topic->m_name == pattern &&
                 topic->m_type == shcore::Topic_type::GLOBAL_OBJECT;
        });
  }

  const shcore::Help_topic *match = nullptr;
  if (index.has_value() && *index != -1) match = topics[*index];

  for (auto topic : topics) {
    if (topic != match) {
      if (groups.find(topic->get_category()->m_name) == groups.end())
        groups[topic->get_category()->m_name] = {};

      groups[topic->get_category()->m_name].insert(topic);
    }
  }

  // The list of fund topic ids wil be displayed
  // By default we exclude the root topic
  auto id_mode = shcore::Topic_id_mode::EXCLUDE_ROOT;
  if (groups.size() <= 1) id_mode = shcore::Topic_id_mode::EXCLUDE_CATEGORIES;

  // If an exact match was found, it's information will be printed right away
  // And the other found topics will be listed into a SEE ALSO section at the
  // end, otherwise the normal printing for multiple topics is used
  std::vector<std::string> output;
  if (match) {
    auto data = _shell->get_helper()->get_help(*match);

    if (!data.empty()) {
      current_console()->println(data);
    }

    current_console()->println();
    output.push_back("<b>SEE ALSO</b>");
    output.push_back("Additional entries were found matching <b>" + pattern +
                     "</b>");
  } else {
    output.push_back("Found several entries matching <b>" + pattern + "</b>");
  }

  for (auto group : groups) {
    output.push_back("The following topics were found at the <b>" +
                     group.first + "</b> category:");

    for (auto topic : group.second) {
      output.push_back("@li " +
                       topic->get_id(_shell->interactive_mode(), id_mode));
    }
  }

  auto topic = *groups.begin()->second.begin();
  output.push_back("For help on a specific topic use: <b>\\?</b> <topic>");
  output.push_back("e.g.: <b>\\?</b> " +
                   topic->get_id(_shell->interactive_mode(), id_mode));

  mysqlsh::current_console()->println(
      textui::format_markup_text(output, 80, 0));
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
    std::vector<const shcore::Help_topic *> global_refs;
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
                                 {}});
      }
    }

    // Inserts the default modules
    global_topics.push_back({"",
                             "mysqlx",
                             shcore::Topic_type::MODULE,
                             "MYSQLX_GLOBAL",
                             nullptr,
                             {},
                             {}});
    global_topics.push_back({"",
                             "mysql",
                             shcore::Topic_type::TOPIC,
                             "MYSQL_GLOBAL",
                             nullptr,
                             {},
                             {}});

    // We need the references to use the existing formatting code
    for (auto &topic : global_topics) global_refs.push_back(&topic);

    std::sort(global_refs.begin(), global_refs.end(),
              shcore::Help_topic_compare());

    sections.push_back(textui::bold("GLOBAL OBJECTS"));
    _shell->get_helper()->add_childs_section(global_refs, &sections, 0, false,
                                             "GLOBALS", "GLOBALS");
    help->add_examples_section("GLOBALS_EXAMPLE_SCRIPTING", &sections, 0);
  } else {
    help->add_examples_section("GLOBALS_EXAMPLE_SQL", &sections, 0);
  }

  mysqlsh::current_console()->println(shcore::str_join(sections, "\n\n"));
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

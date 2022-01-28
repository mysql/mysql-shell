/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#include "modules/util/dump/dump_options.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <mutex>

#include <algorithm>
#include <iterator>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {

namespace {

template <typename T, typename R = T>
const R &key(const T &t) {
  return t;
}

template <typename T, typename R = T>
const R &value(const T &t) {
  return t;
}

template <typename T1, typename T2>
const T1 &key(const std::pair<T1, T2> &t) {
  return t.first;
}

template <typename T1, typename T2>
const T2 &value(const std::pair<T1, T2> &t) {
  return t.second;
}

template <typename T, typename U = T>
void find_matches(const T &included, const T &excluded,
                  const std::string &prefix,
                  const std::function<void(const U &, const U &,
                                           const std::string &)> &callback) {
  const auto included_is_smaller = included.size() < excluded.size();
  const auto &needle = included_is_smaller ? included : excluded;
  const auto &haystack = !included_is_smaller ? included : excluded;

  for (const auto &object : needle) {
    const auto &k = key(object);
    const auto found = haystack.find(k);

    if (haystack.end() != found) {
      auto full_name = prefix;

      if (!full_name.empty()) {
        full_name += ".";
      }

      full_name += shcore::quote_identifier(k);

      callback(value(included_is_smaller ? object : *found),
               value(!included_is_smaller ? object : *found), full_name);
    }
  }
}

}  // namespace

Dump_options::Dump_options()
    : m_show_progress(isatty(fileno(stdout)) ? true : false) {}

const shcore::Option_pack_def<Dump_options> &Dump_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Dump_options>()
          .on_start(&Dump_options::on_start_unpack)
          .optional("maxRate", &Dump_options::set_string_option)
          .optional("showProgress", &Dump_options::m_show_progress)
          .optional("compression", &Dump_options::set_string_option)
          .optional("defaultCharacterSet", &Dump_options::m_character_set)
          .on_log(&Dump_options::on_log_options);

  return opts;
}

void Dump_options::on_start_unpack(const shcore::Dictionary_t &options) {
  m_options = options;
}

void Dump_options::set_string_option(const std::string &option,
                                     const std::string &value) {
  if (option == "maxRate") {
    if (!value.empty()) {
      m_max_rate = mysqlshdk::utils::expand_to_bytes(value);
    }
  } else if (option == "compression") {
    if (value.empty()) {
      throw std::invalid_argument(
          "The option 'compression' cannot be set to an empty string.");
    }

    m_compression = mysqlshdk::storage::to_compression(value);
  } else {
    // This function should only be called with the options above.
    assert(false);
  }
}

void Dump_options::set_storage_config(
    const std::shared_ptr<mysqlshdk::storage::Config> &storage_config) {
  m_storage_config = storage_config;
}

void Dump_options::on_log_options(const char *msg) const {
  log_info("Dump options: %s", msg);
}

void Dump_options::set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  m_session = session;

  on_set_session(session);
}

void Dump_options::validate() const {
  // cross-filter conflicts cannot be checked earlier, as filters are not
  // initialized at the same time
  error_on_schema_cross_filters_conflicts(included_tables(), excluded_tables(),
                                          "a table", "Tables");
  error_on_schema_cross_filters_conflicts(included_events(), excluded_events(),
                                          "an event", "Events");
  error_on_schema_cross_filters_conflicts(
      included_routines(), excluded_routines(), "a routine", "Routines");
  error_on_table_cross_filters_conflicts();

  if (m_filter_conflicts) {
    throw std::invalid_argument("Conflicting filtering options");
  }

  validate_options();
}

bool Dump_options::exists(const std::string &schema) const {
  return find_missing({schema}).empty();
}

bool Dump_options::exists(const std::string &schema,
                          const std::string &table) const {
  return find_missing(schema, {table}).empty();
}

std::set<std::string> Dump_options::find_missing(
    const std::unordered_set<std::string> &schemas) const {
  return find_missing_impl(
      "SELECT SCHEMA_NAME AS name FROM information_schema.schemata", schemas);
}

std::set<std::string> Dump_options::find_missing(
    const std::string &schema,
    const std::unordered_set<std::string> &tables) const {
  return find_missing_impl(
      shcore::sqlstring("SELECT TABLE_NAME AS name "
                        "FROM information_schema.tables WHERE TABLE_SCHEMA = ?",
                        0)
          << schema,
      tables);
}

std::set<std::string> Dump_options::find_missing_impl(
    const std::string &subquery,
    const std::unordered_set<std::string> &objects) const {
  std::string query =
      "SELECT n.name FROM (SELECT " +
      shcore::str_join(objects, " UNION SELECT ",
                       [&objects](const std::string &v) {
                         return (shcore::sqlstring("?", 0) << v).str() +
                                (v == *objects.begin() ? " AS name" : "");
                       });

  query += ") AS n LEFT JOIN (" + subquery +
           ") AS o ON STRCMP(o.name COLLATE utf8_bin, n.name)=0 "
           "WHERE o.name IS NULL";

  const auto result = session()->query_log_error(query);
  std::set<std::string> missing;

  while (const auto row = result->fetch_one()) {
    missing.emplace(row->get_string(0));
  }

  return missing;
}

void Dump_options::error_on_schema_filters_conflicts() const {
  error_on_object_filters_conflicts(included_schemas(), excluded_schemas(),
                                    "a schema", "Schemas", {});
}

void Dump_options::error_on_table_filters_conflicts() const {
  error_on_object_filters_conflicts(included_tables(), excluded_tables(),
                                    "a table", "Tables");
}

void Dump_options::error_on_event_filters_conflicts() const {
  error_on_object_filters_conflicts(included_events(), excluded_events(),
                                    "an event", "Events");
}

void Dump_options::error_on_routine_filters_conflicts() const {
  error_on_object_filters_conflicts(included_routines(), excluded_routines(),
                                    "a routine", "Routines");
}

void Dump_options::error_on_object_filters_conflicts(
    const Instance_cache_builder::Filter &included,
    const Instance_cache_builder::Filter &excluded,
    const std::string &object_label, const std::string &option_suffix,
    const std::string &schema_name) const {
  find_matches<Instance_cache_builder::Filter, std::string>(
      included, excluded, schema_name,
      [&object_label, &option_suffix, this](const auto &, auto &,
                                            const std::string &full_name) {
        m_filter_conflicts = true;
        current_console()->print_error(
            "Both include" + option_suffix + " and exclude" + option_suffix +
            " options contain " + object_label + " " + full_name + ".");
      });
}

void Dump_options::error_on_object_filters_conflicts(
    const Instance_cache_builder::Object_filters &included,
    const Instance_cache_builder::Object_filters &excluded,
    const std::string &object_label, const std::string &option_suffix) const {
  find_matches<Instance_cache_builder::Object_filters,
               Instance_cache_builder::Filter>(
      included, excluded, {},
      [&object_label, &option_suffix, this](const auto &i, const auto &e,
                                            const std::string &schema_name) {
        error_on_object_filters_conflicts(i, e, object_label, option_suffix,
                                          schema_name);
      });
}

void Dump_options::error_on_trigger_filters_conflicts() const {
  find_matches<Instance_cache_builder::Trigger_filters,
               Instance_cache_builder::Object_filters>(
      included_triggers(), excluded_triggers(), {},
      [this](const auto &included, const auto &excluded,
             const std::string &schema_name) {
        find_matches<Instance_cache_builder::Object_filters,
                     Instance_cache_builder::Filter>(
            included, excluded, schema_name,
            [this](const auto &i, const auto &e,
                   const std::string &table_name) {
              if (!i.empty() && !e.empty()) {
                // both trigger-level filters are not empty, check for conflicts
                this->error_on_object_filters_conflicts(i, e, "a trigger",
                                                        "Triggers", table_name);
              } else if (i.empty() != e.empty()) {
                // one of the trigger-level filters is empty, if it's the
                // excluded one, it's a conflict
                if (e.empty()) {
                  m_filter_conflicts = true;

                  const auto console = current_console();

                  for (const auto &trigger : i) {
                    console->print_error(
                        "The includeTriggers option contains a trigger " +
                        table_name + "." + shcore::quote_identifier(trigger) +
                        " which is excluded by the value of the "
                        "excludeTriggers option: " +
                        table_name + ".");
                  }
                }
              } else {
                // both trigger-level filters are empty, this is a conflict
                m_filter_conflicts = true;
                current_console()->print_error(
                    "Both includeTriggers and excludeTriggers options contain "
                    "a filter " +
                    table_name + ".");
              }
            });
      });
}

template <typename C>
void Dump_options::error_on_schema_cross_filters_conflicts(
    const C &included, const C &excluded, const std::string &object_label,
    const std::string &option_suffix) const {
  const auto check_schemas = [this, &object_label, &option_suffix](
                                 const auto &list, bool is_included) {
    const std::string prefix = is_included ? "include" : "exclude";

    for (const auto &schema : list) {
      // excluding an object from an excluded schema is redundant, but not an
      // error
      if (is_included && excluded_schemas().count(schema.first) > 0) {
        m_filter_conflicts = true;

        for (const auto &object : schema.second) {
          current_console()->print_error(
              "The " + prefix + option_suffix + " option contains " +
              object_label + " " + shcore::quote_identifier(schema.first) +
              "." + shcore::quote_identifier(object) +
              " which refers to an excluded schema.");
        }
      }

      if (!included_schemas().empty() &&
          0 == included_schemas().count(schema.first)) {
        m_filter_conflicts = true;

        for (const auto &object : schema.second) {
          current_console()->print_error(
              "The " + prefix + option_suffix + " option contains " +
              object_label + " " + shcore::quote_identifier(schema.first) +
              "." + shcore::quote_identifier(object) +
              " which refers to a schema which was not included in the dump.");
        }
      }
    }
  };

  check_schemas(included, true);
  check_schemas(excluded, false);
}

void Dump_options::error_on_table_cross_filters_conflicts() const {
  const auto print_error = [](const auto &triggers, const std::string &format) {
    const auto console = current_console();

    if (triggers.empty()) {
      console->print_error(shcore::str_format(format.c_str(), "filter", ""));
    } else {
      for (const auto &trigger : triggers) {
        console->print_error(shcore::str_format(
            format.c_str(), "trigger",
            ("." + shcore::quote_identifier(trigger)).c_str()));
      }
    }
  };

  const auto check_schemas = [this, &print_error](const auto &list,
                                                  bool included) {
    const std::string prefix = included ? "include" : "exclude";

    for (const auto &schema : list) {
      // excluding an object from an excluded schema is redundant, but not an
      // error
      if (included && excluded_schemas().count(schema.first) > 0) {
        m_filter_conflicts = true;

        for (const auto &table : schema.second) {
          print_error(table.second,
                      "The " + prefix + "Triggers option contains a %s " +
                          shcore::quote_identifier(schema.first) + "." +
                          shcore::quote_identifier(table.first) +
                          "%s which refers to an excluded schema.");
        }
      }

      if (!included_schemas().empty() &&
          0 == included_schemas().count(schema.first)) {
        m_filter_conflicts = true;

        for (const auto &table : schema.second) {
          print_error(table.second,
                      "The " + prefix + "Triggers option contains a %s " +
                          shcore::quote_identifier(schema.first) + "." +
                          shcore::quote_identifier(table.first) +
                          "%s which refers to a schema which was not included "
                          "in the dump.");
        }
      }
    }
  };

  check_schemas(included_triggers(), true);
  check_schemas(excluded_triggers(), false);

  const auto check_triggers = [this, &print_error](const auto &list,
                                                   bool included) {
    const std::string prefix = included ? "include" : "exclude";

    const auto check_tables = [this, &prefix, &print_error](
                                  const auto &expected, const auto &actual) {
      for (const auto &table : expected) {
        if (0 == actual.second.count(table.first)) {
          m_filter_conflicts = true;
          print_error(table.second,
                      "The " + prefix + "Triggers option contains a %s " +
                          shcore::quote_identifier(actual.first) + "." +
                          shcore::quote_identifier(table.first) +
                          "%s which refers to a table which was not included "
                          "in the dump.");
        }
      }
    };

    for (const auto &schema : list) {
      // excluding a trigger from an excluded table is redundant, but not an
      // error
      if (included) {
        const auto excluded_schema = excluded_tables().find(schema.first);

        if (excluded_tables().end() != excluded_schema) {
          for (const auto &table : schema.second) {
            if (excluded_schema->second.count(table.first) > 0) {
              m_filter_conflicts = true;
              print_error(table.second,
                          "The " + prefix + "Triggers option contains a %s " +
                              shcore::quote_identifier(schema.first) + "." +
                              shcore::quote_identifier(table.first) +
                              "%s which refers to an excluded table.");
            }
          }
        }
      }

      const auto included_schema = included_tables().find(schema.first);

      if (included_tables().end() == included_schema) {
        if (!included_tables().empty()) {
          // included tables filter is not empty, but the schema of the included
          // trigger was not found there, report all tables from that schema as
          // missing
          check_tables(
              schema.second,
              std::make_pair(schema.first, Instance_cache_builder::Filter{}));
        }
      } else {
        check_tables(schema.second, *included_schema);
      }
    }
  };

  check_triggers(included_triggers(), true);
  check_triggers(excluded_triggers(), false);
}

}  // namespace dump
}  // namespace mysqlsh

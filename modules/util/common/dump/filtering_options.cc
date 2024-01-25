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

#include "modules/util/common/dump/filtering_options.h"

#include <algorithm>
#include <cassert>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_filtering.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/util/common/dump/utils.h"

namespace mysqlsh {
namespace dump {
namespace common {

namespace {

using Argument = std::unordered_set<std::string>;

bool error_on_object_filters_conflicts(
    const Filtering_options::Object_filters::Filter &included,
    const Filtering_options::Object_filters::Filter &excluded,
    const std::string &object_label, const std::string &option_suffix) {
  bool has_conflicts = false;

  mysqlshdk::utils::find_matches<Filtering_options::Object_filters::Filter,
                                 Filtering_options::Schema_filters::Filter>(
      included, excluded, {},
      [&object_label, &option_suffix, &has_conflicts](
          const auto &i, const auto &e, const std::string &schema_name) {
        has_conflicts |= mysqlshdk::utils::error_on_conflicts<
            Filtering_options::Schema_filters::Filter, std::string>(
            i, e, object_label, option_suffix, schema_name);
      });

  return has_conflicts;
}

void parse_trigger(const std::string &trigger, const std::string &action,
                   std::string *out_schema, std::string *out_table,
                   std::string *out_trigger) {
  assert(out_schema && out_table && out_trigger);
  std::string schema;
  std::string table;
  std::string object;
  try {
    shcore::split_schema_table_and_object(trigger, &schema, &table, &object);
  } catch (const std::runtime_error &e) {
    throw std::invalid_argument("Failed to parse trigger to be " + action +
                                "d '" + trigger + "': " + e.what());
  }

  if (table.empty()) {
    throw std::invalid_argument(
        "The trigger to be " + action +
        "d must be in the following form: schema.table or "
        "schema.table.trigger, with optional backtick quotes, wrong value: "
        "'" +
        trigger + "'.");
  }

  if (schema.empty()) {
    // we got schema.table, need to move names around
    std::swap(schema, table);
    std::swap(table, object);
  }

  *out_schema = std::move(schema);
  *out_table = std::move(table);
  *out_trigger = std::move(object);
}

}  // namespace

const shcore::Option_pack_def<Filtering_options::User_filters>
    &Filtering_options::User_filters::options() {
  static const auto opts =
      shcore::Option_pack_def<User_filters>()
          .optional("excludeUsers", &User_filters::exclude<Argument>)
          .optional("includeUsers", &User_filters::include<Argument>);

  return opts;
}

bool Filtering_options::User_filters::is_included(
    const shcore::Account &account) const {
  const auto predicate = [&account](const shcore::Account &a) {
    return a.user == account.user &&
           (a.host.empty() ? true : a.host == account.host);
  };

  if (m_excluded.end() !=
      std::find_if(m_excluded.begin(), m_excluded.end(), predicate)) {
    return false;
  }

  if (m_included.empty()) {
    return true;
  }

  return m_included.end() !=
         std::find_if(m_included.begin(), m_included.end(), predicate);
}

bool Filtering_options::User_filters::is_included(
    const std::string &account) const {
  return is_included(shcore::split_account(account));
}

void Filtering_options::User_filters::exclude(shcore::Account account) {
  m_excluded.emplace_back(std::move(account));
}

void Filtering_options::User_filters::include(shcore::Account account) {
  m_included.emplace_back(std::move(account));
}

bool Filtering_options::User_filters::error_on_conflicts() const {
  const auto console = current_console();
  bool conflict = false;

  for (const auto &included : included()) {
    for (const auto &excluded : excluded()) {
      if (included.user == excluded.user) {
        if (included.host == excluded.host) {
          conflict = true;
          console->print_error(
              "Both includeUsers and excludeUsers options contain a user " +
              shcore::make_account(included) + ".");
          // exact match, need to check the remaining excluded users only if the
          // included host is not empty, as we may find the conflict handled
          // below
          if (included.host.empty()) {
            break;
          }
        } else if (excluded.host.empty()) {
          conflict = true;
          console->print_error(
              "The includeUsers option contains a user " +
              shcore::make_account(included) +
              " which is excluded by the value of the excludeUsers option: " +
              shcore::make_account(excluded) + ".");
          // fully specified included account has been excluded, but there can
          // be another conflict, if this account is also explicitly excluded,
          // check the remaining excluded users
        }
        // else, the included host is empty and excluded host is not, a valid
        // scenario; or, the hosts are different -> no conflict
      }
    }
  }

  return conflict;
}

const shcore::Option_pack_def<Filtering_options::Schema_filters>
    &Filtering_options::Schema_filters::options() {
  static const auto opts =
      shcore::Option_pack_def<Schema_filters>()
          .optional("excludeSchemas", &Schema_filters::exclude<Argument>)
          .optional("includeSchemas", &Schema_filters::include<Argument>);

  return opts;
}

bool Filtering_options::Schema_filters::is_included(
    const std::string &schema) const {
  if (m_excluded.count(schema) > 0) return false;

  if (m_included.empty() || m_included.count(schema) > 0) {
    return true;
  }

  return false;
}

bool Filtering_options::Schema_filters::matches_included(
    const std::string &pattern) const {
  const auto matches = [&pattern](const Filter &f) {
    return f.end() !=
           std::find_if(f.begin(), f.end(), [&pattern](const auto &s) {
             return shcore::match_sql_wild(s, pattern);
           });
  };

  if (matches(m_excluded)) return false;

  if (m_included.empty() || matches(m_included)) {
    return true;
  }

  return false;
}

void Filtering_options::Schema_filters::exclude(std::string schema) {
  m_excluded.emplace(std::move(schema));
}

void Filtering_options::Schema_filters::include(std::string schema) {
  m_included.emplace(std::move(schema));
}

void Filtering_options::Schema_filters::exclude(const char *schema) {
  exclude(std::string{schema});
}

void Filtering_options::Schema_filters::include(const char *schema) {
  include(std::string{schema});
}

bool Filtering_options::Schema_filters::error_on_conflicts() const {
  return mysqlshdk::utils::error_on_conflicts<
      Filtering_options::Schema_filters::Filter, std::string>(
      included(), excluded(), "a schema", "Schemas", {});
}

std::string Filtering_options::Schema_filters::parse_schema(
    const std::string &schema) {
  // we allow both quoted and unquoted schema names, if removing quotes fails,
  // we assume that it's an identifier which should have been quoted, but wasn't
  try {
    return shcore::unquote_identifier(schema);
  } catch (const std::runtime_error &e) {
    return schema;
  }
}

Filtering_options::Object_filters::Object_filters(const Schema_filters *schemas,
                                                  const char *object_type,
                                                  const char *object_label,
                                                  const char *option_suffix)
    : m_schemas(schemas),
      m_object_type(object_type),
      m_object_label(object_label),
      m_option_suffix(option_suffix) {}

bool Filtering_options::Object_filters::is_included(
    const std::string &schema, const std::string &object) const {
  assert(!schema.empty());
  assert(!object.empty());

  if (!m_schemas->is_included(schema)) return false;

  if (const auto s = m_excluded.find(schema); s != m_excluded.end()) {
    if (const auto o = s->second.find(object); o != s->second.end()) {
      return false;
    }
  }

  if (m_included.empty()) {
    return true;
  }

  if (const auto s = m_included.find(schema); s != m_included.end()) {
    if (const auto o = s->second.find(object); o != s->second.end()) {
      return true;
    }
  }

  return false;
}

void Filtering_options::Object_filters::exclude(
    const std::string &qualified_object) {
  std::string schema;
  std::string object;

  parse_schema_and_object(qualified_object, m_object_type + " to be excluded",
                          m_object_type, &schema, &object);
  exclude(std::move(schema), std::move(object));
}

void Filtering_options::Object_filters::include(
    const std::string &qualified_object) {
  std::string schema;
  std::string object;

  parse_schema_and_object(qualified_object, m_object_type + " to be included",
                          m_object_type, &schema, &object);
  include(std::move(schema), std::move(object));
}

void Filtering_options::Object_filters::exclude(const char *qualified_object) {
  exclude(std::string{qualified_object});
}

void Filtering_options::Object_filters::include(const char *qualified_object) {
  include(std::string{qualified_object});
}

void Filtering_options::Object_filters::exclude(std::string schema,
                                                std::string object) {
  m_excluded[std::move(schema)].emplace(std::move(object));
}

void Filtering_options::Object_filters::include(std::string schema,
                                                std::string object) {
  m_included[std::move(schema)].emplace(std::move(object));
}

void Filtering_options::Object_filters::exclude(std::string schema,
                                                const char *object) {
  exclude(std::move(schema), std::string{object});
}

void Filtering_options::Object_filters::include(std::string schema,
                                                const char *object) {
  include(std::move(schema), std::string{object});
}

bool Filtering_options::Object_filters::error_on_conflicts() const {
  return error_on_object_filters_conflicts(included(), excluded(),
                                           m_object_label, m_option_suffix);
}

bool Filtering_options::Object_filters::error_on_cross_filters_conflicts()
    const {
  bool has_conflicts = false;
  const auto check_schemas = [this, &has_conflicts](const auto &list,
                                                    bool is_included) {
    const std::string prefix = is_included ? "include" : "exclude";

    for (const auto &schema : list) {
      // excluding an object from an excluded schema is redundant, but not an
      // error
      if (is_included && m_schemas->excluded().count(schema.first) > 0) {
        has_conflicts = true;

        for (const auto &object : schema.second) {
          current_console()->print_error(
              "The " + prefix + m_option_suffix + " option contains " +
              m_object_label + " " + shcore::quote_identifier(schema.first) +
              "." + shcore::quote_identifier(object) +
              " which refers to an excluded schema.");
        }
      }

      if (!m_schemas->included().empty() &&
          0 == m_schemas->included().count(schema.first)) {
        has_conflicts = true;

        for (const auto &object : schema.second) {
          current_console()->print_error(
              "The " + prefix + m_option_suffix + " option contains " +
              m_object_label + " " + shcore::quote_identifier(schema.first) +
              "." + shcore::quote_identifier(object) +
              " which refers to a schema which was not included in the dump.");
        }
      }
    }
  };

  check_schemas(included(), true);
  check_schemas(excluded(), false);

  return has_conflicts;
}

Filtering_options::Table_filters::Table_filters(const Schema_filters *schemas)
    : Object_filters(schemas, "table", "a table", "Tables") {}

const shcore::Option_pack_def<Filtering_options::Table_filters>
    &Filtering_options::Table_filters::options() {
  static const auto opts =
      shcore::Option_pack_def<Table_filters>()
          .optional("excludeTables", &Table_filters::exclude<Argument>)
          .optional("includeTables", &Table_filters::include<Argument>);

  return opts;
}

Filtering_options::Event_filters::Event_filters(const Schema_filters *schemas)
    : Object_filters(schemas, "event", "an event", "Events") {}

const shcore::Option_pack_def<Filtering_options::Event_filters>
    &Filtering_options::Event_filters::options() {
  static const auto opts =
      shcore::Option_pack_def<Event_filters>()
          .optional("excludeEvents", &Event_filters::exclude<Argument>)
          .optional("includeEvents", &Event_filters::include<Argument>);

  return opts;
}

Filtering_options::Routine_filters::Routine_filters(
    const Schema_filters *schemas)
    : Object_filters(schemas, "routine", "a routine", "Routines") {}

const shcore::Option_pack_def<Filtering_options::Routine_filters>
    &Filtering_options::Routine_filters::options() {
  static const auto opts =
      shcore::Option_pack_def<Routine_filters>()
          .optional("excludeRoutines", &Routine_filters::exclude<Argument>)
          .optional("includeRoutines", &Routine_filters::include<Argument>);

  return opts;
}

bool Filtering_options::Routine_filters::is_included_ci(
    const std::string &schema, const std::string &routine) const {
  if (!m_schemas->is_included(schema)) return false;

  // case-sensitive check, in order to avoid character set conversions
  if (is_included(schema, routine)) {
    return true;
  }

  const auto r = shcore::utf8_to_wide(routine);
  const auto contains_ci = [&r](const auto &c) {
    for (const auto &e : c) {
      if (shcore::str_caseeq(r, shcore::utf8_to_wide(e))) {
        return true;
      }
    }

    return false;
  };

  if (const auto s = m_excluded.find(schema); s != m_excluded.end()) {
    if (contains_ci(s->second)) {
      return false;
    }
  }

  if (m_included.empty()) {
    return true;
  }

  if (const auto s = m_included.find(schema); s != m_included.end()) {
    if (contains_ci(s->second)) {
      return true;
    }
  }

  return false;
}

Filtering_options::Trigger_filters::Trigger_filters(
    const Schema_filters *schemas, const Table_filters *tables)
    : m_schemas(schemas), m_tables(tables) {}

const shcore::Option_pack_def<Filtering_options::Trigger_filters>
    &Filtering_options::Trigger_filters::options() {
  static const auto opts =
      shcore::Option_pack_def<Trigger_filters>()
          .optional("excludeTriggers", &Trigger_filters::exclude<Argument>)
          .optional("includeTriggers", &Trigger_filters::include<Argument>);

  return opts;
}

bool Filtering_options::Trigger_filters::is_included(
    const std::string &schema, const std::string &table,
    const std::string &trigger) const {
  assert(!schema.empty());
  assert(!table.empty());
  assert(!trigger.empty());

  if (!m_tables->is_included(schema, table)) return false;

  if (const auto s = m_excluded.find(schema); s != m_excluded.end()) {
    if (const auto t = s->second.find(table); t != s->second.end()) {
      if (t->second.empty()) {
        // an empty set, all triggers are excluded
        return false;
      }

      if (const auto o = t->second.find(trigger); o != t->second.end()) {
        return false;
      }
    }
  }

  if (m_included.empty()) {
    return true;
  }

  if (const auto s = m_included.find(schema); s != m_included.end()) {
    if (const auto t = s->second.find(table); t != s->second.end()) {
      if (t->second.empty()) {
        // an empty set, all triggers are included
        return true;
      }

      if (const auto o = t->second.find(trigger); o != t->second.end()) {
        return true;
      }
    }
  }

  return false;
}

void Filtering_options::Trigger_filters::exclude(
    const std::string &qualified_object) {
  std::string schema;
  std::string table;
  std::string trigger;

  parse_trigger(qualified_object, "exclude", &schema, &table, &trigger);
  // Object name can be empty, in this case all triggers in a table are
  // included/excluded. We add it as is, so later we can detect cases like
  // 'schema.table' and 'schema.table.trigger' given by the user at the same
  // time.
  exclude(std::move(schema), std::move(table), std::move(trigger));
}

void Filtering_options::Trigger_filters::include(
    const std::string &qualified_object) {
  std::string schema;
  std::string table;
  std::string trigger;

  parse_trigger(qualified_object, "include", &schema, &table, &trigger);
  include(std::move(schema), std::move(table), std::move(trigger));
}

void Filtering_options::Trigger_filters::exclude(const char *qualified_object) {
  exclude(std::string{qualified_object});
}

void Filtering_options::Trigger_filters::include(const char *qualified_object) {
  include(std::string{qualified_object});
}

void Filtering_options::Trigger_filters::exclude(std::string schema,
                                                 std::string table,
                                                 std::string trigger) {
  add(std::move(schema), std::move(table), std::move(trigger), &m_excluded);
}

void Filtering_options::Trigger_filters::include(std::string schema,
                                                 std::string table,
                                                 std::string trigger) {
  add(std::move(schema), std::move(table), std::move(trigger), &m_included);
}

bool Filtering_options::Trigger_filters::error_on_conflicts() const {
  bool has_conflicts = false;

  mysqlshdk::utils::find_matches<Trigger_filters::Filter,
                                 Object_filters::Filter>(
      included(), excluded(), {},
      [&has_conflicts](const auto &included, const auto &excluded,
                       const std::string &schema_name) {
        mysqlshdk::utils::find_matches<Object_filters::Filter,
                                       Schema_filters::Filter>(
            included, excluded, schema_name,
            [&has_conflicts](const auto &i, const auto &e,
                             const std::string &table_name) {
              if (!i.empty() && !e.empty()) {
                // both trigger-level filters are not empty, check for conflicts
                has_conflicts |=
                    mysqlshdk::utils::error_on_conflicts<Schema_filters::Filter,
                                                         std::string>(
                        i, e, "a trigger", "Triggers", table_name);
              } else if (i.empty() != e.empty()) {
                // one of the trigger-level filters is empty, if it's the
                // excluded one, it's a conflict
                if (e.empty()) {
                  has_conflicts = true;

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
                has_conflicts = true;
                current_console()->print_error(
                    "Both includeTriggers and excludeTriggers options contain "
                    "a filter " +
                    table_name + ".");
              }
            });
      });

  return has_conflicts;
}

bool Filtering_options::Trigger_filters::error_on_cross_filters_conflicts()
    const {
  bool has_conflicts = false;

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

  const auto check_schemas = [this, &has_conflicts, &print_error](
                                 const auto &list, bool included) {
    const std::string prefix = included ? "include" : "exclude";

    for (const auto &schema : list) {
      // excluding an object from an excluded schema is redundant, but not an
      // error
      if (included && m_schemas->excluded().count(schema.first) > 0) {
        has_conflicts = true;

        for (const auto &table : schema.second) {
          print_error(table.second,
                      "The " + prefix + "Triggers option contains a %s " +
                          shcore::quote_identifier(schema.first) + "." +
                          shcore::quote_identifier(table.first) +
                          "%s which refers to an excluded schema.");
        }
      }

      if (!m_schemas->included().empty() &&
          0 == m_schemas->included().count(schema.first)) {
        has_conflicts = true;

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

  check_schemas(included(), true);
  check_schemas(excluded(), false);

  const auto check_triggers = [this, &has_conflicts, &print_error](
                                  const auto &list, bool included) {
    const std::string prefix = included ? "include" : "exclude";

    const auto check_tables = [&has_conflicts, &prefix, &print_error](
                                  const auto &expected, const auto &actual) {
      for (const auto &table : expected) {
        if (0 == actual.second.count(table.first)) {
          has_conflicts = true;
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
        const auto excluded_schema = m_tables->excluded().find(schema.first);

        if (m_tables->excluded().end() != excluded_schema) {
          for (const auto &table : schema.second) {
            if (excluded_schema->second.count(table.first) > 0) {
              has_conflicts = true;
              print_error(table.second,
                          "The " + prefix + "Triggers option contains a %s " +
                              shcore::quote_identifier(schema.first) + "." +
                              shcore::quote_identifier(table.first) +
                              "%s which refers to an excluded table.");
            }
          }
        }
      }

      const auto included_schema = m_tables->included().find(schema.first);

      if (m_tables->included().end() == included_schema) {
        if (!m_tables->included().empty()) {
          // included tables filter is not empty, but the schema of the included
          // trigger was not found there, report all tables from that schema as
          // missing
          check_tables(schema.second,
                       std::make_pair(schema.first, Schema_filters::Filter{}));
        }
      } else {
        check_tables(schema.second, *included_schema);
      }
    }
  };

  check_triggers(included(), true);
  check_triggers(excluded(), false);

  return has_conflicts;
}

void Filtering_options::Trigger_filters::add(std::string schema,
                                             std::string table,
                                             std::string trigger,
                                             Filter *target) {
  auto it = (*target)[std::move(schema)].try_emplace(std::move(table));

  if (trigger.empty()) {
    // an empty trigger name, clear the set, all triggers from this table are
    // included/excluded
    it.first->second.clear();
  } else {
    // non-empty trigger name
    if (it.second || !it.first->second.empty()) {
      // add trigger if table entry was just created or if there are other names
      // there
      it.first->second.emplace(std::move(trigger));
    }  // else an empty set means that all triggers are included/excluded
  }
}

Filtering_options::Filtering_options()
    : m_tables(&m_schemas),
      m_events(&m_schemas),
      m_routines(&m_schemas),
      m_triggers(&m_schemas, &m_tables) {}

Filtering_options::Filtering_options(const Filtering_options &other)
    : Filtering_options() {
  *this = other;
}

Filtering_options::Filtering_options(Filtering_options &&other)
    : Filtering_options() {
  *this = std::move(other);
}

Filtering_options &Filtering_options::operator=(
    const Filtering_options &other) {
  m_users = other.m_users;
  m_schemas = other.m_schemas;
  m_tables = other.m_tables;
  m_events = other.m_events;
  m_routines = other.m_routines;
  m_triggers = other.m_triggers;

  update_pointers();

  return *this;
}

Filtering_options &Filtering_options::operator=(Filtering_options &&other) {
  m_users = std::move(other.m_users);
  m_schemas = std::move(other.m_schemas);
  m_tables = std::move(other.m_tables);
  m_events = std::move(other.m_events);
  m_routines = std::move(other.m_routines);
  m_triggers = std::move(other.m_triggers);

  update_pointers();

  return *this;
}

void Filtering_options::update_pointers() {
  m_tables.m_schemas = &m_schemas;
  m_events.m_schemas = &m_schemas;
  m_routines.m_schemas = &m_schemas;
  m_triggers.m_schemas = &m_schemas;
  m_triggers.m_tables = &m_tables;
}

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

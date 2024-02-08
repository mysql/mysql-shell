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

#ifndef MODULES_UTIL_COMMON_DUMP_FILTERING_OPTIONS_H_
#define MODULES_UTIL_COMMON_DUMP_FILTERING_OPTIONS_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/types_cpp.h"

#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dump {
namespace common {

class Filtering_options final {
 public:
  class User_filters {
   public:
    using Filter = std::vector<shcore::Account>;

    User_filters() = default;

    User_filters(const User_filters &) = default;
    User_filters(User_filters &&) = default;

    User_filters &operator=(const User_filters &) = default;
    User_filters &operator=(User_filters &&) = default;

    ~User_filters() = default;

    static const shcore::Option_pack_def<User_filters> &options();

    const Filter &excluded() const { return m_excluded; }

    const Filter &included() const { return m_included; }

    bool is_included(const shcore::Account &account) const;

    bool is_included(const std::string &account) const;

    template <typename C>
    inline void exclude(const C &users) {
      add(users, &m_excluded);
    }

    template <typename C>
    inline void include(const C &users) {
      add(users, &m_included);
    }

    void exclude(shcore::Account account);

    void include(shcore::Account account);

    bool error_on_conflicts() const;

   private:
    template <typename C>
    void add(const C &users, Filter *target) {
      try {
        auto accounts = shcore::to_accounts(users);
        target->insert(target->end(), std::make_move_iterator(accounts.begin()),
                       std::make_move_iterator(accounts.end()));
      } catch (const std::runtime_error &e) {
        throw std::invalid_argument(e.what());
      }
    }

    Filter m_included;
    Filter m_excluded;
  };

  class Schema_filters {
   public:
    using Filter = std::unordered_set<std::string>;

    Schema_filters() = default;

    Schema_filters(const Schema_filters &) = default;
    Schema_filters(Schema_filters &&) = default;

    Schema_filters &operator=(const Schema_filters &) = default;
    Schema_filters &operator=(Schema_filters &&) = default;

    ~Schema_filters() = default;

    static const shcore::Option_pack_def<Schema_filters> &options();

    const Filter &excluded() const { return m_excluded; }

    const Filter &included() const { return m_included; }

    bool is_included(const std::string &schema) const;

    /**
     * Checks if the given pattern matches an included schema (case-sensitive).
     */
    bool matches_included(const std::string &pattern) const;

    template <typename C>
    void exclude(const C &schemas) {
      for (const auto &s : schemas) {
        exclude(parse_schema(s));
      }
    }

    template <typename C>
    void include(const C &schemas) {
      for (const auto &s : schemas) {
        include(parse_schema(s));
      }
    }

    void exclude(std::string schema);

    void include(std::string schema);

    void exclude(const char *schema);

    void include(const char *schema);

    bool error_on_conflicts() const;

   private:
    static std::string parse_schema(const std::string &schema);

    Filter m_included;
    Filter m_excluded;
  };

  class Object_filters {
   public:
    using Filter = std::unordered_map<std::string, Schema_filters::Filter>;

    Object_filters() = delete;

    Object_filters(const Object_filters &) = default;
    Object_filters(Object_filters &&) = default;

    Object_filters &operator=(const Object_filters &) = default;
    Object_filters &operator=(Object_filters &&) = default;

    ~Object_filters() = default;

    const Filter &excluded() const { return m_excluded; }

    const Filter &included() const { return m_included; }

    bool is_included(const std::string &schema,
                     const std::string &object) const;

    template <typename C>
    void exclude(const C &objects) {
      for (const auto &o : objects) {
        exclude(o);
      }
    }

    template <typename C>
    void include(const C &objects) {
      for (const auto &o : objects) {
        include(o);
      }
    }

    void exclude(const std::string &qualified_object);

    void include(const std::string &qualified_object);

    void exclude(const char *qualified_object);

    void include(const char *qualified_object);

    void exclude(std::string schema, std::string object);

    void include(std::string schema, std::string object);

    void exclude(std::string schema, const char *object);

    void include(std::string schema, const char *object);

    template <typename C>
    void exclude(std::string schema, const C &objects) {
      for (const auto &o : objects) {
        exclude(schema, o);
      }
    }

    template <typename C>
    void include(std::string schema, const C &objects) {
      for (const auto &o : objects) {
        include(schema, o);
      }
    }

    bool error_on_conflicts() const;

    bool error_on_cross_filters_conflicts() const;

   protected:
    Object_filters(const Schema_filters *schemas, const char *object_type,
                   const char *object_label, const char *option_suffix);

   private:
    friend class Filtering_options;

    const Schema_filters *m_schemas;
    std::string m_object_type;
    std::string m_object_label;
    std::string m_option_suffix;
    Filter m_included;
    Filter m_excluded;
  };

  class Table_filters : public Object_filters {
   public:
    using Object_filters::Object_filters;

    explicit Table_filters(const Schema_filters *schemas);

    static const shcore::Option_pack_def<Table_filters> &options();
  };

  class Event_filters : public Object_filters {
   public:
    using Object_filters::Object_filters;

    explicit Event_filters(const Schema_filters *schemas);

    static const shcore::Option_pack_def<Event_filters> &options();
  };

  class Routine_filters : public Object_filters {
   public:
    using Object_filters::Object_filters;

    explicit Routine_filters(const Schema_filters *schemas);

    static const shcore::Option_pack_def<Routine_filters> &options();

    bool is_included_ci(const std::string &schema,
                        const std::string &routine) const;
  };

  class Trigger_filters {
   public:
    using Filter = std::unordered_map<std::string, Object_filters::Filter>;

    Trigger_filters() = delete;

    Trigger_filters(const Schema_filters *schemas, const Table_filters *tables);

    Trigger_filters(const Trigger_filters &) = default;
    Trigger_filters(Trigger_filters &&) = default;

    Trigger_filters &operator=(const Trigger_filters &) = default;
    Trigger_filters &operator=(Trigger_filters &&) = default;

    ~Trigger_filters() = default;

    static const shcore::Option_pack_def<Trigger_filters> &options();

    const Filter &excluded() const { return m_excluded; }

    const Filter &included() const { return m_included; }

    bool is_included(const std::string &schema, const std::string &table,
                     const std::string &trigger) const;

    template <typename C>
    void exclude(const C &objects) {
      for (const auto &o : objects) {
        exclude(o);
      }
    }

    template <typename C>
    void include(const C &objects) {
      for (const auto &o : objects) {
        include(o);
      }
    }

    void exclude(const std::string &qualified_object);

    void include(const std::string &qualified_object);

    void exclude(const char *qualified_object);

    void include(const char *qualified_object);

    void exclude(std::string schema, std::string table, std::string trigger);

    void include(std::string schema, std::string table, std::string trigger);

    bool error_on_conflicts() const;

    bool error_on_cross_filters_conflicts() const;

   private:
    friend class Filtering_options;

    void add(std::string schema, std::string table, std::string trigger,
             Filter *target);

    const Schema_filters *m_schemas;
    const Table_filters *m_tables;
    Filter m_included;
    Filter m_excluded;
  };

  Filtering_options();

  Filtering_options(const Filtering_options &other);
  Filtering_options(Filtering_options &&other);

  Filtering_options &operator=(const Filtering_options &other);

  Filtering_options &operator=(Filtering_options &&other);

  ~Filtering_options() = default;

  User_filters &users() { return m_users; }
  const User_filters &users() const { return m_users; }

  Schema_filters &schemas() { return m_schemas; }
  const Schema_filters &schemas() const { return m_schemas; }

  Table_filters &tables() { return m_tables; }
  const Table_filters &tables() const { return m_tables; }

  Event_filters &events() { return m_events; }
  const Event_filters &events() const { return m_events; }

  Routine_filters &routines() { return m_routines; }
  const Routine_filters &routines() const { return m_routines; }

  Trigger_filters &triggers() { return m_triggers; }
  const Trigger_filters &triggers() const { return m_triggers; }

 private:
  void update_pointers();

  User_filters m_users;
  Schema_filters m_schemas;
  Table_filters m_tables;
  Event_filters m_events;
  Routine_filters m_routines;
  Trigger_filters m_triggers;
};

}  // namespace common
}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_DUMP_FILTERING_OPTIONS_H_

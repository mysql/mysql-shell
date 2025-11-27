/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_DUMP_COMPATIBILITY_ISSUE_H_
#define MODULES_UTIL_DUMP_COMPATIBILITY_ISSUE_H_

#include <set>
#include <string>
#include <string_view>

#include "modules/util/dump/compatibility_option.h"

namespace mysqlsh {
namespace dump {

enum class Compatibility_check {
  USER_UNSUPPORTED_AUTH_PLUGIN,
  USER_DEPRECATED_AUTH_PLUGIN,
  USER_NO_PASSWORD,
  USER_RESTRICTED_GRANTS,
  USER_INVALID_GRANTS,
  USER_WILDCARD_GRANT,
  USER_ESCAPED_WILDCARD_GRANT,
  USER_GRANT_ON_MISSING_OBJECT,
  USER_GRANT_ON_MISSING_ROLE,

  SCHEMA_ENCRYPTION,

  VIEW_INVALID_REFERENCE,
  VIEW_MISMATCHED_REFERENCE,
  VIEW_INVALID_DEFINITION,

  TABLE_UNSUPPORTED_ENGINE,
  TABLE_CANNOT_REPLACE_ENGINE,
  TABLE_MISSING_PK,
  TABLE_DATA_OR_INDEX_DIR,
  TABLE_ENCRYPTION,
  TABLE_TABLESPACE,
  TABLE_UNSUPPORTED_ROW_FORMAT,
  TABLE_TOO_MANY_COLUMNS,

  OBJECT_RESTRICTED_DEFINER,
  OBJECT_INVALID_DEFINER,
  OBJECT_INVALID_DEFINER_USERS_NOT_DUMPED,
  OBJECT_INVALID_DEFINER_MISSING_USER,
  OBJECT_MISSING_SQL_SECURITY,
  OBJECT_MISSING_SQL_SECURITY_AND_DEFINER,
  OBJECT_COLLATION_UNSUPPORTED,
  OBJECT_COLLATION_REPLACED,

  ROUTINE_MISSING_DEPENDENCY,
};

struct Compatibility_issue {
  enum class Status {
    FIXED,
    NOTE,
    WARNING,
    ERROR,
  };

  enum class Object_type {
    UNSPECIFIED,
    USER,
    SCHEMA,
    TABLE,
    VIEW,
    COLUMN,
    FUNCTION,
    PROCEDURE,
    PARAMETER,
    RETURN_VALUE,
    EVENT,
    TRIGGER,
  };

  struct fixed {
    static Compatibility_issue user_unsupported_auth_plugin_removed(
        const std::string &user, const std::string &plugin);

    static Compatibility_issue user_unsupported_auth_plugin_locked(
        const std::string &user, const std::string &plugin);

    static Compatibility_issue user_no_password_removed(
        const std::string &user);

    static Compatibility_issue user_no_password_locked(const std::string &user);

    static Compatibility_issue user_restricted_grants(
        const std::string &user, const std::set<std::string> &grants);

    static Compatibility_issue user_restricted_grants_on_mysql(
        const std::string &user, std::string_view grant);

    static Compatibility_issue user_restricted_grants_replaced(
        const std::string &user, std::string_view from, std::string_view to);

    static Compatibility_issue user_invalid_grants(const std::string &user,
                                                   const char *object_type,
                                                   const std::string &grant);

    static Compatibility_issue user_wildcard_grant(const std::string &user,
                                                   const std::string &grant);

    static Compatibility_issue user_escaped_wildcard_grant(
        const std::string &user, const std::string &schema,
        const std::string &new_schema);

    static Compatibility_issue schema_encryption(const std::string &schema);

    static Compatibility_issue table_unsupported_engine(
        const std::string &table, const std::string &engine);

    static Compatibility_issue table_missing_pk_create(
        const std::string &table);

    static Compatibility_issue table_missing_pk_ignore(
        const std::string &table);

    static Compatibility_issue table_data_or_index_dir(
        const std::string &table);

    static Compatibility_issue table_encryption(const std::string &table);

    static Compatibility_issue table_tablespace(const std::string &table);

    static Compatibility_issue table_unsupported_row_format(
        const std::string &table);

    static Compatibility_issue object_invalid_definer(Object_type type,
                                                      const std::string &name);

    static Compatibility_issue object_missing_sql_security(
        Object_type type, const std::string &name);
  };

  struct note {
    static Compatibility_issue user_grant_on_missing_role(
        const std::string &user, const std::string &role,
        const std::string &grant);

    static Compatibility_issue routine_missing_dependency(
        Object_type type, const std::string &routine,
        const std::string &reference);
  };

  struct warning {
    static Compatibility_issue user_deprecated_auth_plugin(
        const std::string &user, const std::string &plugin);

    static Compatibility_issue user_escaped_wildcard_grant(
        const std::string &user, const std::string &schema);

    static Compatibility_issue user_grant_on_missing_object(
        const std::string &user, const std::string &grant);

    static Compatibility_issue user_grant_on_missing_role(
        const std::string &user, const std::string &role,
        const std::string &grant);

    static Compatibility_issue view_invalid_reference(
        const std::string &view, const std::string &reference);

    static Compatibility_issue view_mismatched_reference(
        const std::string &view, const std::string &reference);

    static Compatibility_issue object_invalid_definer(Object_type type,
                                                      const std::string &name,
                                                      const std::string &user);

    static Compatibility_issue object_invalid_definer_users_not_dumped();

    static Compatibility_issue object_invalid_definer_missing_user(
        Object_type type, const std::string &name, const std::string &user);

    static Compatibility_issue object_missing_sql_security(
        Object_type type, const std::string &name);

    static Compatibility_issue object_collation_replaced(
        Object_type object_type, const std::string &object_name,
        const char *collation_type, std::string_view from, std::string_view to);

    static Compatibility_issue routine_missing_dependency(
        Object_type type, const std::string &routine,
        const std::string &reference);
  };

  struct error {
    static Compatibility_issue user_unsupported_auth_plugin(
        const std::string &user, const std::string &plugin);

    static Compatibility_issue user_no_password(const std::string &user);

    static Compatibility_issue user_restricted_grants(
        const std::string &user, const std::set<std::string> &grants);

    static Compatibility_issue user_restricted_grants_on_mysql(
        const std::string &user, std::string_view grant);

    static Compatibility_issue user_invalid_grants(const std::string &user,
                                                   const char *object_type,
                                                   const std::string &grant);

    static Compatibility_issue user_wildcard_grant(const std::string &user,
                                                   const std::string &grant);

    static Compatibility_issue view_mismatched_reference(
        const std::string &view, const std::string &reference);

    static Compatibility_issue view_invalid_definition(const std::string &view);

    static Compatibility_issue table_unsupported_engine(
        const std::string &table, const std::string &engine);

    static Compatibility_issue table_cannot_replace_engine(
        const std::string &table, const std::string &engine,
        const std::string &error);

    static Compatibility_issue table_missing_pk(const std::string &table);

    static Compatibility_issue table_missing_pk_manual_fix(
        const std::string &table, const char *reason);

    static Compatibility_issue table_tablespace(const std::string &table);

    static Compatibility_issue table_unsupported_row_format(
        const std::string &table);

    static Compatibility_issue table_too_many_columns(const std::string &table,
                                                      std::size_t count,
                                                      std::size_t limit);

    static Compatibility_issue object_restricted_definer(
        Object_type type, const std::string &name, const std::string &user);

    static Compatibility_issue object_invalid_definer(Object_type type,
                                                      const std::string &name,
                                                      const std::string &user);

    static Compatibility_issue object_missing_sql_security(
        Object_type type, const std::string &name);

    static Compatibility_issue object_missing_sql_security_and_definer(
        Object_type type, const std::string &name);

    static Compatibility_issue object_unsupported_collation(
        Object_type object_type, const std::string &object_name,
        const char *collation_type, std::string_view collation);
  };

  bool operator==(const Compatibility_issue &i) const = default;

  Compatibility_check check;
  Status status;
  Object_type object_type;
  std::string object_name;
  std::string description;
  Compatibility_options compatibility_options;

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(Schema_dumper_test,
              check_object_for_definer_set_any_definer_issues);
  FRIEND_TEST(Schema_dumper_test, strip_restricted_grants_set_any_definer);
#endif  // FRIEND_TEST

  Compatibility_issue() = default;

  Compatibility_issue(Compatibility_check c, Status s, Object_type t,
                      const std::string &o)
      : check(c), status(s), object_type(t), object_name(o) {}

  struct common {
    static inline Compatibility_issue user(Compatibility_check c, Status s,
                                           const std::string &u) {
      return {c, s, Object_type::USER, u};
    }

    static Compatibility_issue user_unsupported_auth_plugin(
        Status s, const std::string &user, const std::string &plugin);

    static Compatibility_issue user_no_password(Status s,
                                                const std::string &user);

    static Compatibility_issue user_restricted_grants(Status s,
                                                      const std::string &user);

    static Compatibility_issue user_invalid_grants(Status s,
                                                   const std::string &user);

    static Compatibility_issue user_wildcard_grant(Status s,
                                                   const std::string &user,
                                                   const std::string &grant);

    static Compatibility_issue user_escaped_wildcard_grant(
        Status s, const std::string &user, const std::string &schema);

    static Compatibility_issue user_grant_on_missing_role(
        Status s, const std::string &user, const std::string &role,
        const std::string &grant);

    static inline Compatibility_issue schema(Compatibility_check c, Status s,
                                             const std::string &name) {
      return {c, s, Object_type::SCHEMA, name};
    }

    static inline Compatibility_issue view(Compatibility_check c, Status s,
                                           const std::string &v) {
      return {c, s, Object_type::VIEW, v};
    }

    static Compatibility_issue view_mismatched_reference(
        Status s, const std::string &view, const std::string &reference);

    static inline Compatibility_issue table(Compatibility_check c, Status s,
                                            const std::string &t) {
      return {c, s, Object_type::TABLE, t};
    }

    static Compatibility_issue table_unsupported_engine(
        Status s, const std::string &table);

    static Compatibility_issue table_missing_pk(Status s,
                                                const std::string &table,
                                                const char *context);

    static Compatibility_issue table_tablespace(Status s,
                                                const std::string &table);

    static Compatibility_issue table_unsupported_row_format(
        Status s, const std::string &table);

    static Compatibility_issue object_invalid_definer(Status s,
                                                      Object_type type,
                                                      const std::string &name);

    static Compatibility_issue object_missing_sql_security(
        Status s, Object_type type, const std::string &name);

    static Compatibility_issue routine_missing_dependency(
        Status s, Object_type type, const std::string &routine,
        const std::string &reference, const char *reason);
  };
};

std::string_view to_string(Compatibility_issue::Status status);
std::string_view to_string(Compatibility_issue::Object_type object);
std::string_view to_string(Compatibility_check check);

}  // namespace dump
}  // namespace mysqlsh

#endif  // MODULES_UTIL_DUMP_COMPATIBILITY_ISSUE_H_

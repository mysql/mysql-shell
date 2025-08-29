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

#include "modules/util/dump/compatibility_issue.h"

#include <cctype>

#include <cassert>

#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dump {

namespace {

void capitalize(std::string *s) {
  if (s->empty()) {
    return;
  }

  (*s)[0] = std::toupper((*s)[0]);
}

}  // namespace

Compatibility_issue Compatibility_issue::common::user_unsupported_auth_plugin(
    Status s, const std::string &user, const std::string &plugin) {
  auto issue =
      common::user(Compatibility_check::USER_UNSUPPORTED_AUTH_PLUGIN, s, user);

  issue.description = shcore::str_format(
      "User %s is using an unsupported authentication plugin '%s'",
      user.c_str(), plugin.c_str());
  issue.compatibility_options |= Compatibility_option::LOCK_INVALID_ACCOUNTS;
  issue.compatibility_options |= Compatibility_option::SKIP_INVALID_ACCOUNTS;

  return issue;
}

Compatibility_issue Compatibility_issue::error::user_unsupported_auth_plugin(
    const std::string &user, const std::string &plugin) {
  return common::user_unsupported_auth_plugin(Status::ERROR, user, plugin);
}

Compatibility_issue
Compatibility_issue::fixed::user_unsupported_auth_plugin_removed(
    const std::string &user, const std::string &plugin) {
  auto issue =
      common::user_unsupported_auth_plugin(Status::FIXED, user, plugin);

  issue.description += ", this account has been removed from the dump";
  issue.compatibility_options.unset(
      Compatibility_option::LOCK_INVALID_ACCOUNTS);

  return issue;
}

Compatibility_issue
Compatibility_issue::fixed::user_unsupported_auth_plugin_locked(
    const std::string &user, const std::string &plugin) {
  auto issue =
      common::user_unsupported_auth_plugin(Status::FIXED, user, plugin);

  issue.description += ", this account has been updated and locked";
  issue.compatibility_options.unset(
      Compatibility_option::SKIP_INVALID_ACCOUNTS);

  return issue;
}

Compatibility_issue Compatibility_issue::warning::user_deprecated_auth_plugin(
    const std::string &user, const std::string &plugin) {
  auto issue = common::user(Compatibility_check::USER_DEPRECATED_AUTH_PLUGIN,
                            Status::WARNING, user);

  issue.description = shcore::str_format(
      "User %s is using a deprecated authentication plugin '%s'", user.c_str(),
      plugin.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::common::user_no_password(
    Status s, const std::string &user) {
  auto issue = common::user(Compatibility_check::USER_NO_PASSWORD, s, user);

  issue.description =
      shcore::str_format("User %s does not have a password set", user.c_str());
  issue.compatibility_options |= Compatibility_option::LOCK_INVALID_ACCOUNTS;
  issue.compatibility_options |= Compatibility_option::SKIP_INVALID_ACCOUNTS;

  return issue;
}

Compatibility_issue Compatibility_issue::error::user_no_password(
    const std::string &user) {
  return common::user_no_password(Status::ERROR, user);
}

Compatibility_issue Compatibility_issue::fixed::user_no_password_removed(
    const std::string &user) {
  auto issue = common::user_no_password(Status::FIXED, user);

  issue.description += ", this account has been removed from the dump";
  issue.compatibility_options.unset(
      Compatibility_option::LOCK_INVALID_ACCOUNTS);

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::user_no_password_locked(
    const std::string &user) {
  auto issue = common::user_no_password(Status::FIXED, user);

  issue.description += ", this account has been updated and locked";
  issue.compatibility_options.unset(
      Compatibility_option::SKIP_INVALID_ACCOUNTS);

  return issue;
}

Compatibility_issue Compatibility_issue::common::user_restricted_grants(
    Status s, const std::string &user) {
  auto issue =
      common::user(Compatibility_check::USER_RESTRICTED_GRANTS, s, user);

  issue.compatibility_options |= Compatibility_option::STRIP_RESTRICTED_GRANTS;

  return issue;
}

Compatibility_issue Compatibility_issue::error::user_restricted_grants(
    const std::string &user, const std::set<std::string> &grants) {
  auto issue = common::user_restricted_grants(Status::ERROR, user);

  issue.description = shcore::str_format(
      "User %s is granted restricted privilege%s: %s", user.c_str(),
      grants.size() > 1 ? "s" : "", shcore::str_join(grants, ", ").c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::user_restricted_grants(
    const std::string &user, const std::set<std::string> &grants) {
  auto issue = common::user_restricted_grants(Status::FIXED, user);

  issue.description = shcore::str_format(
      "User %s had restricted privilege%s (%s) removed", user.c_str(),
      grants.size() > 1 ? "s" : "", shcore::str_join(grants, ", ").c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::error::user_restricted_grants_on_mysql(
    const std::string &user, std::string_view grant) {
  auto issue = common::user_restricted_grants(Status::ERROR, user);

  issue.description = shcore::str_format(
      "User %s has explicit grants on mysql schema object: %.*s", user.c_str(),
      static_cast<int>(grant.size()), grant.data());

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::user_restricted_grants_on_mysql(
    const std::string &user, std::string_view grant) {
  auto issue = common::user_restricted_grants(Status::FIXED, user);

  issue.description = shcore::str_format(
      "User %s had explicit grants on mysql schema object %.*s removed",
      user.c_str(), static_cast<int>(grant.size()), grant.data());

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::user_restricted_grants_replaced(
    const std::string &user, std::string_view from, std::string_view to) {
  auto issue = common::user_restricted_grants(Status::FIXED, user);

  issue.description = shcore::str_format(
      "User %s had restricted privilege %.*s replaced with %.*s", user.c_str(),
      static_cast<int>(from.size()), from.data(), static_cast<int>(to.size()),
      to.data());

  return issue;
}

Compatibility_issue Compatibility_issue::common::user_invalid_grants(
    Status s, const std::string &user) {
  auto issue = common::user(Compatibility_check::USER_INVALID_GRANTS, s, user);

  issue.compatibility_options |= Compatibility_option::STRIP_INVALID_GRANTS;

  return issue;
}

Compatibility_issue Compatibility_issue::error::user_invalid_grants(
    const std::string &user, const char *object_type,
    const std::string &grant) {
  auto issue = common::user_invalid_grants(Status::ERROR, user);

  issue.description = shcore::str_format(
      "User %s has grant statement on a non-existent %s (%s)", user.c_str(),
      object_type, grant.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::user_invalid_grants(
    const std::string &user, const char *object_type,
    const std::string &grant) {
  auto issue = common::user_invalid_grants(Status::FIXED, user);

  issue.description = shcore::str_format(
      "User %s had grant statement on a non-existent %s removed (%s)",
      user.c_str(), object_type, grant.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::common::user_wildcard_grant(
    Status s, const std::string &user, const std::string &grant) {
  auto issue = common::user(Compatibility_check::USER_WILDCARD_GRANT, s, user);

  issue.description = shcore::str_format(
      "User %s has a wildcard grant statement at the database level (%s)",
      user.c_str(), grant.c_str());
  issue.compatibility_options |= Compatibility_option::IGNORE_WILDCARD_GRANTS;

  return issue;
}

Compatibility_issue Compatibility_issue::error::user_wildcard_grant(
    const std::string &user, const std::string &grant) {
  return common::user_wildcard_grant(Status::ERROR, user, grant);
}

Compatibility_issue Compatibility_issue::fixed::user_wildcard_grant(
    const std::string &user, const std::string &grant) {
  auto issue = common::user_wildcard_grant(Status::FIXED, user, grant);

  issue.description += ", this issue is ignored";

  return issue;
}

Compatibility_issue Compatibility_issue::common::user_escaped_wildcard_grant(
    Status s, const std::string &user, const std::string &schema) {
  auto issue =
      common::user(Compatibility_check::USER_ESCAPED_WILDCARD_GRANT, s, user);

  issue.description = shcore::str_format(
      "User %s has a wildcard grant statement at the database level which is "
      "using escaped wildcard characters (%s)",
      user.c_str(), schema.c_str());
  issue.compatibility_options |= Compatibility_option::UNESCAPE_WILDCARD_GRANTS;

  return issue;
}

Compatibility_issue Compatibility_issue::warning::user_escaped_wildcard_grant(
    const std::string &user, const std::string &schema) {
  return common::user_escaped_wildcard_grant(Status::WARNING, user, schema);
}

Compatibility_issue Compatibility_issue::fixed::user_escaped_wildcard_grant(
    const std::string &user, const std::string &schema,
    const std::string &new_schema) {
  auto issue = common::user_escaped_wildcard_grant(Status::FIXED, user, schema);

  issue.description += shcore::str_format(
      ", schema name has been replaced with: %s", new_schema.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::warning::user_grant_on_missing_object(
    const std::string &user, const std::string &grant) {
  auto issue = common::user(Compatibility_check::USER_GRANT_ON_MISSING_OBJECT,
                            Status::WARNING, user);

  issue.description = shcore::str_format(
      "User %s has a grant statement on an object which is not included in the "
      "dump (%s)",
      user.c_str(), grant.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::common::user_grant_on_missing_role(
    Status s, const std::string &user, const std::string &role,
    const std::string &grant) {
  auto issue =
      common::user(Compatibility_check::USER_GRANT_ON_MISSING_ROLE, s, user);

  issue.description = shcore::str_format(
      "User %s has a grant statement on a role %s which is not included in the "
      "dump (%s)",
      user.c_str(), role.c_str(), grant.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::warning::user_grant_on_missing_role(
    const std::string &user, const std::string &role,
    const std::string &grant) {
  return common::user_grant_on_missing_role(Status::WARNING, user, role, grant);
}

Compatibility_issue Compatibility_issue::note::user_grant_on_missing_role(
    const std::string &user, const std::string &role,
    const std::string &grant) {
  return common::user_grant_on_missing_role(Status::NOTE, user, role, grant);
}

Compatibility_issue Compatibility_issue::fixed::schema_encryption(
    const std::string &schema) {
  auto issue = common::schema(Compatibility_check::SCHEMA_ENCRYPTION,
                              Status::FIXED, schema);

  issue.description = shcore::str_format(
      "Database %s had unsupported ENCRYPTION option commented out",
      schema.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::warning::view_invalid_reference(
    const std::string &view, const std::string &reference) {
  auto issue = common::view(Compatibility_check::VIEW_INVALID_REFERENCE,
                            Status::WARNING, view);

  issue.description = shcore::str_format(
      "View %s references table/view %s which is not included in the dump",
      view.c_str(), reference.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::common::view_mismatched_reference(
    Status s, const std::string &view, const std::string &reference) {
  auto issue =
      common::view(Compatibility_check::VIEW_MISMATCHED_REFERENCE, s, view);

  issue.description = shcore::str_format(
      "View %s references table/view %s which was not found using "
      "case-sensitive search",
      view.c_str(), reference.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::error::view_mismatched_reference(
    const std::string &view, const std::string &reference) {
  return common::view_mismatched_reference(Status::ERROR, view, reference);
}

Compatibility_issue Compatibility_issue::warning::view_mismatched_reference(
    const std::string &view, const std::string &reference) {
  return common::view_mismatched_reference(Status::WARNING, view, reference);
}

Compatibility_issue Compatibility_issue::common::table_unsupported_engine(
    Status s, const std::string &table) {
  auto issue =
      common::table(Compatibility_check::TABLE_UNSUPPORTED_ENGINE, s, table);

  issue.compatibility_options |= Compatibility_option::FORCE_INNODB;

  return issue;
}

Compatibility_issue Compatibility_issue::error::table_unsupported_engine(
    const std::string &table, const std::string &engine) {
  auto issue = common::table_unsupported_engine(Status::ERROR, table);

  issue.description =
      shcore::str_format("Table %s uses unsupported storage engine %s",
                         table.c_str(), engine.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::table_unsupported_engine(
    const std::string &table, const std::string &engine) {
  auto issue = common::table_unsupported_engine(Status::FIXED, table);

  issue.description =
      shcore::str_format("Table %s had unsupported engine %s changed to InnoDB",
                         table.c_str(), engine.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::common::table_missing_pk(
    Status s, const std::string &table, const char *context) {
  auto issue = common::table(Compatibility_check::TABLE_MISSING_PK, s, table);

  issue.description = shcore::str_format(
      "Table %s does not have a Primary Key, %s", table.c_str(), context);
  issue.compatibility_options |= Compatibility_option::CREATE_INVISIBLE_PKS;
  issue.compatibility_options |= Compatibility_option::IGNORE_MISSING_PKS;

  return issue;
}

Compatibility_issue Compatibility_issue::error::table_missing_pk(
    const std::string &table) {
  return common::table_missing_pk(
      Status::ERROR, table,
      "which is required for High Availability in MySQL HeatWave Service");
}

Compatibility_issue Compatibility_issue::fixed::table_missing_pk_create(
    const std::string &table) {
  auto issue = common::table_missing_pk(
      Status::FIXED, table, "this will be fixed when the dump is loaded");

  issue.compatibility_options.unset(Compatibility_option::IGNORE_MISSING_PKS);

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::table_missing_pk_ignore(
    const std::string &table) {
  auto issue =
      common::table_missing_pk(Status::FIXED, table, "this is ignored");

  issue.compatibility_options.unset(Compatibility_option::CREATE_INVISIBLE_PKS);

  return issue;
}

Compatibility_issue Compatibility_issue::error::table_missing_pk_manual_fix(
    const std::string &table, const char *reason) {
  auto issue = common::table_missing_pk(
      Status::ERROR, table,
      "this cannot be fixed automatically because the table ");

  issue.description += reason;
  issue.compatibility_options.clear();

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::table_data_or_index_dir(
    const std::string &table) {
  auto issue = common::table(Compatibility_check::TABLE_DATA_OR_INDEX_DIR,
                             Status::FIXED, table);

  issue.description = shcore::str_format(
      "Table %s had {DATA|INDEX} DIRECTORY table option commented out",
      table.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::table_encryption(
    const std::string &table) {
  auto issue = common::table(Compatibility_check::TABLE_ENCRYPTION,
                             Status::FIXED, table);

  issue.description = shcore::str_format(
      "Table %s had ENCRYPTION table option commented out", table.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::common::table_tablespace(
    Status s, const std::string &table) {
  auto issue = common::table(Compatibility_check::TABLE_TABLESPACE, s, table);

  issue.compatibility_options |= Compatibility_option::STRIP_TABLESPACES;

  return issue;
}

Compatibility_issue Compatibility_issue::error::table_tablespace(
    const std::string &table) {
  auto issue = common::table_tablespace(Status::ERROR, table);

  issue.description = shcore::str_format(
      "Table %s uses unsupported tablespace option", table.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::table_tablespace(
    const std::string &table) {
  auto issue = common::table_tablespace(Status::FIXED, table);

  issue.description = shcore::str_format(
      "Table %s had unsupported tablespace option removed", table.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::common::table_unsupported_row_format(
    Status s, const std::string &table) {
  auto issue = common::table(Compatibility_check::TABLE_UNSUPPORTED_ROW_FORMAT,
                             s, table);

  issue.compatibility_options |= Compatibility_option::FORCE_INNODB;

  return issue;
}

Compatibility_issue Compatibility_issue::error::table_unsupported_row_format(
    const std::string &table) {
  auto issue = common::table_unsupported_row_format(Status::ERROR, table);

  issue.description = shcore::str_format(
      "Table %s uses unsupported ROW_FORMAT=FIXED option", table.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::table_unsupported_row_format(
    const std::string &table) {
  auto issue = common::table_unsupported_row_format(Status::FIXED, table);

  issue.description = shcore::str_format(
      "Table %s had unsupported ROW_FORMAT=FIXED option removed",
      table.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::error::table_too_many_columns(
    const std::string &table, std::size_t count, std::size_t limit) {
  auto issue = common::table(Compatibility_check::TABLE_TOO_MANY_COLUMNS,
                             Status::ERROR, table);

  issue.description = shcore::str_format(
      "Table %s has %zu columns, while the limit for the InnoDB engine is %zu "
      "columns",
      table.c_str(), count, limit);

  return issue;
}

Compatibility_issue Compatibility_issue::error::object_restricted_definer(
    Object_type type, const std::string &name, const std::string &user) {
  Compatibility_issue issue{Compatibility_check::OBJECT_RESTRICTED_DEFINER,
                            Status::ERROR, type, name};

  const auto object_type = to_string(type);
  issue.description = shcore::str_format(
      "%.*s %s - definition uses DEFINER clause set to user %s whose user name "
      "is restricted in MySQL HeatWave Service",
      static_cast<int>(object_type.size()), object_type.data(), name.c_str(),
      user.c_str());
  capitalize(&issue.description);

  return issue;
}

Compatibility_issue Compatibility_issue::common::object_invalid_definer(
    Status s, Object_type type, const std::string &name) {
  Compatibility_issue issue{Compatibility_check::OBJECT_INVALID_DEFINER, s,
                            type, name};

  const auto object_type = to_string(type);
  issue.description =
      shcore::str_format("%.*s %s ", static_cast<int>(object_type.size()),
                         object_type.data(), name.c_str());
  capitalize(&issue.description);

  issue.compatibility_options |= Compatibility_option::STRIP_DEFINERS;

  return issue;
}

Compatibility_issue Compatibility_issue::error::object_invalid_definer(
    Object_type type, const std::string &name, const std::string &user) {
  auto issue = common::object_invalid_definer(Status::ERROR, type, name);

  issue.description += shcore::str_format(
      "- definition uses DEFINER clause set to user %s which can only be "
      "executed by this user or a user with SET_ANY_DEFINER, SET_USER_ID or "
      "SUPER privileges",
      user.c_str());

  return issue;
}

Compatibility_issue Compatibility_issue::warning::object_invalid_definer(
    Object_type type, const std::string &name, const std::string &user) {
  auto issue = error::object_invalid_definer(type, name, user);

  issue.status = Status::WARNING;

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::object_invalid_definer(
    Object_type type, const std::string &name) {
  auto issue = common::object_invalid_definer(Status::FIXED, type, name);

  issue.description += "had definer clause removed";

  return issue;
}

Compatibility_issue
Compatibility_issue::warning::object_invalid_definer_users_not_dumped() {
  Compatibility_issue issue{
      Compatibility_check::OBJECT_INVALID_DEFINER_USERS_NOT_DUMPED,
      Status::WARNING,
      Object_type::UNSPECIFIED,
      {}};

  issue.description =
      "One or more DDL statements contain DEFINER clause but user information "
      "is not included in the dump. Loading will fail if accounts set as "
      "definers do not already exist in the target DB System instance.";

  return issue;
}

Compatibility_issue
Compatibility_issue::warning::object_invalid_definer_missing_user(
    Object_type type, const std::string &name, const std::string &user) {
  Compatibility_issue issue{
      Compatibility_check::OBJECT_INVALID_DEFINER_MISSING_USER, Status::WARNING,
      type, name};

  const auto object_type = to_string(type);
  issue.description = shcore::str_format(
      "%.*s %s - definition uses DEFINER clause set to user %s which does not "
      "exist or is not included",
      static_cast<int>(object_type.size()), object_type.data(), name.c_str(),
      user.c_str());
  capitalize(&issue.description);

  return issue;
}

Compatibility_issue Compatibility_issue::common::object_missing_sql_security(
    Status s, Object_type type, const std::string &name) {
  Compatibility_issue issue{Compatibility_check::OBJECT_MISSING_SQL_SECURITY, s,
                            type, name};

  const auto object_type = to_string(type);
  issue.description =
      shcore::str_format("%.*s %s ", static_cast<int>(object_type.size()),
                         object_type.data(), name.c_str());
  capitalize(&issue.description);

  issue.compatibility_options |= Compatibility_option::STRIP_DEFINERS;

  return issue;
}

Compatibility_issue Compatibility_issue::error::object_missing_sql_security(
    Object_type type, const std::string &name) {
  auto issue = common::object_missing_sql_security(Status::ERROR, type, name);

  issue.description +=
      "- definition does not use SQL SECURITY INVOKER characteristic, which is "
      "mandatory when the DEFINER clause is omitted or removed";

  return issue;
}

Compatibility_issue Compatibility_issue::warning::object_missing_sql_security(
    Object_type type, const std::string &name) {
  auto issue = error::object_missing_sql_security(type, name);

  issue.status = Status::WARNING;

  return issue;
}

Compatibility_issue Compatibility_issue::fixed::object_missing_sql_security(
    Object_type type, const std::string &name) {
  auto issue = common::object_missing_sql_security(Status::FIXED, type, name);

  issue.description += "had SQL SECURITY characteristic set to INVOKER";

  return issue;
}

Compatibility_issue
Compatibility_issue::error::object_missing_sql_security_and_definer(
    Object_type type, const std::string &name) {
  Compatibility_issue issue{
      Compatibility_check::OBJECT_MISSING_SQL_SECURITY_AND_DEFINER,
      Status::ERROR, type, name};

  const auto object_type = to_string(type);
  issue.description = shcore::str_format(
      "%.*s %s - definition does not have a DEFINER clause and does not use "
      "SQL SECURITY INVOKER characteristic, either one of these is required",
      static_cast<int>(object_type.size()), object_type.data(), name.c_str());
  capitalize(&issue.description);

  return issue;
}

Compatibility_issue Compatibility_issue::error::object_unsupported_collation(
    Object_type object_type, const std::string &object_name,
    const char *collation_type, std::string_view collation) {
  Compatibility_issue issue{Compatibility_check::OBJECT_COLLATION_UNSUPPORTED,
                            Status::ERROR, object_type, object_name};

  const auto type = to_string(object_type);
  issue.description = shcore::str_format(
      "%.*s %s has %s set to unsupported collation '%.*s'",
      static_cast<int>(type.size()), type.data(), object_name.c_str(),
      collation_type, static_cast<int>(collation.size()), collation.data());
  capitalize(&issue.description);

  return issue;
}

Compatibility_issue Compatibility_issue::warning::object_collation_replaced(
    Object_type object_type, const std::string &object_name,
    const char *collation_type, std::string_view from, std::string_view to) {
  Compatibility_issue issue{Compatibility_check::OBJECT_COLLATION_REPLACED,
                            Status::WARNING, object_type, object_name};

  const auto type = to_string(object_type);
  issue.description = shcore::str_format(
      "%.*s %s had %s set to '%.*s', it has been replaced with '%.*s'",
      static_cast<int>(type.size()), type.data(), object_name.c_str(),
      collation_type, static_cast<int>(from.size()), from.data(),
      static_cast<int>(to.size()), to.data());
  capitalize(&issue.description);

  return issue;
}

Compatibility_issue Compatibility_issue::common::routine_missing_dependency(
    Status s, Object_type type, const std::string &routine,
    const std::string &reference, const char *reason) {
  Compatibility_issue issue{Compatibility_check::ROUTINE_MISSING_DEPENDENCY, s,
                            type, routine};

  const auto object_type = to_string(type);
  issue.description = shcore::str_format(
      "%.*s %s references library %s which %s",
      static_cast<int>(object_type.size()), object_type.data(), routine.c_str(),
      reference.c_str(), reason);
  capitalize(&issue.description);

  return issue;
}

Compatibility_issue Compatibility_issue::warning::routine_missing_dependency(
    Object_type type, const std::string &routine,
    const std::string &reference) {
  return common::routine_missing_dependency(Status::WARNING, type, routine,
                                            reference, "does not exist");
}

Compatibility_issue Compatibility_issue::note::routine_missing_dependency(
    Object_type type, const std::string &routine,
    const std::string &reference) {
  return common::routine_missing_dependency(
      Status::NOTE, type, routine, reference, "is not included in the dump");
}

std::string_view to_string(Compatibility_issue::Status status) {
  switch (status) {
    case Compatibility_issue::Status::FIXED:
      return "FIXED";

    case Compatibility_issue::Status::NOTE:
      return "NOTE";

    case Compatibility_issue::Status::WARNING:
      return "WARNING";

    case Compatibility_issue::Status::ERROR:
      return "ERROR";
  }

  assert(false);
  return {};
}

std::string_view to_string(Compatibility_issue::Object_type object) {
  switch (object) {
    case Compatibility_issue::Object_type::UNSPECIFIED:
      return "unspecified";

    case Compatibility_issue::Object_type::USER:
      return "user";

    case Compatibility_issue::Object_type::SCHEMA:
      return "schema";

    case Compatibility_issue::Object_type::TABLE:
      return "table";

    case Compatibility_issue::Object_type::VIEW:
      return "view";

    case Compatibility_issue::Object_type::COLUMN:
      return "column";

    case Compatibility_issue::Object_type::FUNCTION:
      return "function";

    case Compatibility_issue::Object_type::PROCEDURE:
      return "procedure";

    case Compatibility_issue::Object_type::PARAMETER:
      return "parameter";

    case Compatibility_issue::Object_type::RETURN_VALUE:
      return "return value of the function";

    case Compatibility_issue::Object_type::EVENT:
      return "event";

    case Compatibility_issue::Object_type::TRIGGER:
      return "trigger";
  }

  assert(false);
  return {};
}

std::string_view to_string(Compatibility_check check) {
  switch (check) {
    case Compatibility_check::USER_UNSUPPORTED_AUTH_PLUGIN:
      return "user/unsupported_auth_plugin";

    case Compatibility_check::USER_DEPRECATED_AUTH_PLUGIN:
      return "user/deprecated_auth_plugin";

    case Compatibility_check::USER_NO_PASSWORD:
      return "user/no_password";

    case Compatibility_check::USER_RESTRICTED_GRANTS:
      return "user/restricted_grants";

    case Compatibility_check::USER_INVALID_GRANTS:
      return "user/invalid_grants";

    case Compatibility_check::USER_WILDCARD_GRANT:
      return "user/wildcard_grant";

    case Compatibility_check::USER_ESCAPED_WILDCARD_GRANT:
      return "user/escaped_wildcard_grant";

    case Compatibility_check::USER_GRANT_ON_MISSING_OBJECT:
      return "user/grant_on_missing_object";

    case Compatibility_check::USER_GRANT_ON_MISSING_ROLE:
      return "user/grant_on_missing_role";

    case Compatibility_check::SCHEMA_ENCRYPTION:
      return "schema/encryption";

    case Compatibility_check::VIEW_INVALID_REFERENCE:
      return "view/invalid_reference";

    case Compatibility_check::VIEW_MISMATCHED_REFERENCE:
      return "view/mismatched_reference";

    case Compatibility_check::TABLE_UNSUPPORTED_ENGINE:
      return "table/unsupported_engine";

    case Compatibility_check::TABLE_MISSING_PK:
      return "table/missing_pk";

    case Compatibility_check::TABLE_DATA_OR_INDEX_DIR:
      return "table/data_or_index_directory";

    case Compatibility_check::TABLE_ENCRYPTION:
      return "table/encryption";

    case Compatibility_check::TABLE_TABLESPACE:
      return "table/tablespace";

    case Compatibility_check::TABLE_UNSUPPORTED_ROW_FORMAT:
      return "table/unsupported_row_format";

    case Compatibility_check::TABLE_TOO_MANY_COLUMNS:
      return "table/too_many_columns";

    case Compatibility_check::OBJECT_RESTRICTED_DEFINER:
      return "object/restricted_definer";

    case Compatibility_check::OBJECT_INVALID_DEFINER:
      return "object/invalid_definer";

    case Compatibility_check::OBJECT_INVALID_DEFINER_USERS_NOT_DUMPED:
      return "object/invalid_definer_users_not_dumped";

    case Compatibility_check::OBJECT_INVALID_DEFINER_MISSING_USER:
      return "object/invalid_definer_missing_user";

    case Compatibility_check::OBJECT_MISSING_SQL_SECURITY:
      return "object/missing_sql_security";

    case Compatibility_check::OBJECT_MISSING_SQL_SECURITY_AND_DEFINER:
      return "object/missing_sql_security_and_definer";

    case Compatibility_check::OBJECT_COLLATION_UNSUPPORTED:
      return "object/object_collation_unsupported";

    case Compatibility_check::OBJECT_COLLATION_REPLACED:
      return "object/object_collation_replaced";

    case Compatibility_check::ROUTINE_MISSING_DEPENDENCY:
      return "routine/missing_dependency";
  }

  assert(false);
  return {};
}

}  // namespace dump
}  // namespace mysqlsh

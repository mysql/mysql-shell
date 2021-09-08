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

#include "mysqlshdk/libs/mysql/utils.h"
#include <mysqld_error.h>
#include <rapidjson/document.h>
#include <algorithm>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_lexing.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace mysql {

static constexpr int kMAX_WEAK_PASSWORD_RETRIES = 100;

namespace detail {

std::string::size_type skip_quoted_thing(const std::string &grant,
                                         std::string::size_type p) {
  if (grant[p] == '\'')
    return utils::span_quoted_string_sq(grant, p);
  else if (grant[p] == '"')
    return utils::span_quoted_string_dq(grant, p);
  else if (grant[p] == '`')
    return utils::span_quoted_sql_identifier_bt(grant, p);
  else
    return std::string::npos;
}

std::string replace_created_user(const std::string &create_user,
                                 const std::string &replacement) {
  std::string::size_type p;

  if (shcore::str_ibeginswith(create_user, "CREATE USER IF NOT EXISTS"))
    p = strlen("CREATE USER IF NOT EXISTS ");
  else
    p = strlen("CREATE USER ");

  auto start = p;
  p = skip_quoted_thing(create_user, p);
  if (p == std::string::npos)
    throw std::logic_error("Could not parse CREATE USER: " + create_user);
  if (create_user[p] != '@')
    throw std::logic_error("Could not parse CREATE USER: " + create_user);
  ++p;
  auto end = p = skip_quoted_thing(create_user, p);
  if (p == std::string::npos)
    throw std::logic_error("Could not parse CREATE USER: " + create_user);

  return create_user.substr(0, start) + replacement + create_user.substr(end);
}

std::string replace_grantee(const std::string &grant,
                            const std::string &replacement) {
  // GRANT .... TO `user`@`host` KEYWORD*
  std::string::size_type p = 0;

  while (p < grant.size()) {
    if (grant.compare(p, 3, "TO ", 0, 3) == 0) {
      p += 2;                                     // skip the TO
      p = grant.find_first_not_of(" \t\n\r", p);  // skip spaces
      auto start = p;
      p = skip_quoted_thing(grant, p);
      if (p == std::string::npos)
        throw std::logic_error("Could not parse GRANT string: " + grant);
      if (grant[p] != '@')
        throw std::logic_error("Could not parse GRANT string: " + grant);
      ++p;
      auto end = p = skip_quoted_thing(grant, p);
      if (p == std::string::npos)
        throw std::logic_error("Could not parse GRANT string: " + grant);

      return grant.substr(0, start) + replacement + grant.substr(end);
    }
    switch (grant[p]) {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        p = grant.find_first_not_of(" \t\n\r", p);
        break;
      case '`':
        p = utils::span_quoted_sql_identifier_bt(grant, p);
        break;
      case '\'':
        p = utils::span_quoted_string_sq(grant, p);
        break;
      case '"':
        p = utils::span_quoted_string_dq(grant, p);
        break;
      default:
        if (std::isalpha(grant[p]))
          p = utils::span_keyword(grant, p);
        else
          ++p;
        break;
    }
  }
  throw std::logic_error("Could not parse GRANT string: " + grant);
}
}  // namespace detail

void drop_view_or_table(const IInstance &instance, const std::string &schema,
                        const std::string &name, bool if_exists) {
  try {
    instance.executef("DROP VIEW !.!", schema.c_str(), name.c_str());
  } catch (const mysqlshdk::db::Error &error) {
    // If the object exists but it's a VIEW mysql returns wrong object error,
    // so drop_table is called
    if (error.code() == ER_WRONG_OBJECT) {
      drop_table_or_view(instance, schema, name, if_exists);
    } else if (error.code() == ER_BAD_TABLE_ERROR) {
      if (!if_exists) throw;
    } else {
      throw;
    }
  }
}

void drop_table_or_view(const IInstance &instance, const std::string &schema,
                        const std::string &name, bool if_exists) {
  try {
    instance.executef("DROP TABLE !.!", schema.c_str(), name.c_str());
  } catch (const mysqlshdk::db::Error &error) {
    if (error.code() == ER_BAD_TABLE_ERROR) {
      drop_view_or_table(instance, schema, name, if_exists);
    } else {
      throw;
    }
  }
}

void clone_user(const IInstance &instance, const std::string &orig_user,
                const std::string &orig_host, const std::string &new_user,
                const std::string &new_host) {
  std::string account = shcore::quote_identifier(new_user) + "@" +
                        shcore::quote_identifier(new_host);

  std::string create_user = instance.queryf_one_string(
      0, "", "SHOW CREATE USER ?@?", orig_user, orig_host);

  instance.execute(detail::replace_created_user(create_user, account));

  std::vector<std::string> grants;
  auto result =
      instance.queryf("SHOW GRANTS FOR /*(*/ ?@? /*)*/", orig_user, orig_host);
  while (auto row = result->fetch_one()) {
    std::string grant = row->get_string(0);
    assert(shcore::str_beginswith(grant, "GRANT "));

    // GRANT PROXY not supported
    assert(!shcore::str_beginswith(grant, "GRANT PROXY"));
    if (shcore::str_beginswith(grant, "GRANT PROXY")) continue;

    // replace original user with new one
    grants.push_back(detail::replace_grantee(grant, account));
  }

  // grant everything
  for (const auto &grant : grants) {
    instance.execute(grant);
  }
}

void clone_user(const IInstance &instance, const std::string &orig_user,
                const std::string &orig_host, const std::string &new_user,
                const std::string &new_host, const std::string &password,
                Account_attribute_set flags) {
  std::string account = shcore::quote_identifier(new_user) + "@" +
                        shcore::quote_identifier(new_host);

  // create user
  instance.executef("CREATE USER /*(*/ ?@? /*)*/ IDENTIFIED BY /*((*/ ? /*))*/",
                    new_user, new_host, password);

  if (flags & Account_attribute::Grants) {
    // gather all grants from the original account
    std::vector<std::string> grants;
    auto result = instance.queryf("SHOW GRANTS FOR /*(*/ ?@? /*)*/", orig_user,
                                  orig_host);
    while (auto row = result->fetch_one()) {
      std::string grant = row->get_string(0);
      assert(shcore::str_beginswith(grant, "GRANT "));

      // Ignore GRANT PROXY
      if (shcore::str_beginswith(grant, "GRANT PROXY")) continue;

      // replace original user with new one
      grants.push_back(detail::replace_grantee(grant, account));
    }

    // grant everything
    for (const auto &grant : grants) {
      instance.execute(grant);
    }
  }
}

Privilege_list get_global_grants(const IInstance &instance,
                                 const std::string &user,
                                 const std::string &host) {
  Privilege_list grants;
  // TODO(alfredo) in 5.7, grantee may have truncated values if username is
  // long
  auto res = instance.queryf(
      "SELECT privilege_type FROM information_schema.user_privileges"
      " WHERE grantee=concat(quote(?), '@', quote(?))",
      user, host);
  while (const auto row = res->fetch_one()) {
    grants.emplace_back(row->get_string(0));
  }
  return grants;
}

std::vector<std::pair<std::string, Privilege_list>> get_user_restrictions(
    const IInstance &instance, const std::string &user,
    const std::string &host) {
  std::vector<std::pair<std::string, Privilege_list>> restrictions;

  auto res = instance.queryf(
      "SELECT User_attributes->>'$.Restrictions' FROM mysql.user WHERE user=? "
      "AND host=?",
      user, host);
  if (auto row = res->fetch_one()) {
    if (!row->is_null(0)) {
      std::string json = row->get_string(0);

      if (!json.empty()) {
        rapidjson::Document doc;
        doc.Parse(json.c_str());

        if (!doc.IsArray())
          throw std::runtime_error(
              "Error parsing account restrictions for user " + user + "." +
              host);

        for (const rapidjson::Value &restr : doc.GetArray()) {
          if (!restr.IsObject() || !restr.HasMember("Database") ||
              !restr.HasMember("Privileges"))
            throw std::runtime_error(
                "Error parsing account restrictions for user " + user + "." +
                host);

          const auto &privs = restr["Privileges"];
          if (!restr["Database"].IsString() || !privs.IsArray())
            throw std::runtime_error(
                "Error parsing account restrictions for user " + user + "." +
                host);

          std::string db = restr["Database"].GetString();
          Privilege_list priv_list;
          for (const auto &priv : privs.GetArray()) {
            if (!priv.IsString())
              throw std::runtime_error(
                  "Error parsing account restrictions for user " + user + "." +
                  host);
            priv_list.push_back(priv.GetString());
          }
          restrictions.emplace_back(db, priv_list);
        }
      }
    }
  } else {
    throw std::runtime_error("User " + user + "." + host + " does not exist");
  }

  return restrictions;
}

void drop_all_accounts_for_user(const IInstance &instance,
                                const std::string &user) {
  std::string accounts;

  auto result = instance.queryf(
      "SELECT concat(quote(user), '@', quote(host)) from mysql.user where "
      "user=?",
      user);
  while (auto row = result->fetch_one()) {
    if (!accounts.empty()) accounts.push_back(',');
    accounts.append(row->get_string(0));
  }

  if (!accounts.empty()) {
    log_debug("Dropping user account '%s' at '%s'", accounts.c_str(),
              instance.descr().c_str());
    instance.execute("DROP USER IF EXISTS " + accounts);
  }
}

std::vector<std::string> get_all_hostnames_for_user(const IInstance &instance,
                                                    const std::string &user) {
  std::vector<std::string> hosts;

  auto result =
      instance.queryf("SELECT host from mysql.user where user=?", user);
  while (auto row = result->fetch_one()) {
    hosts.push_back(row->get_string(0));
  }
  return hosts;
}

/*
 * Generates a random passwrd
 * with length equal to PASSWORD_LENGTH
 */
std::string generate_password(size_t password_length) {
  std::random_device rd;
  static const char *alpha_lower = "abcdefghijklmnopqrstuvwxyz";
  static const char *alpha_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static const char *special_chars = "~@#%$^&*()-_=+]}[{|;:.>,</?";
  static const char *numeric = "1234567890";

  assert(kPASSWORD_LENGTH >= 20);

  if (password_length < kPASSWORD_LENGTH) password_length = kPASSWORD_LENGTH;

  auto get_random = [&rd](size_t size, const char *source) {
    std::string data;

    std::uniform_int_distribution<int> dist_num(0, strlen(source) - 1);

    for (size_t i = 0; i < size; i++) {
      char random = source[dist_num(rd)];

      // Make sure there are no consecutive values
      if (i == 0) {
        data += random;
      } else {
        if (random != data[i - 1])
          data += random;
        else
          i--;
      }
    }

    return data;
  };

  // Generate a passwrd

  // Fill the passwrd string with special chars
  std::string pwd = get_random(password_length, special_chars);

  // Replace 8 random non-consecutive chars by
  // 4 upperCase alphas and 4 lowerCase alphas
  std::string alphas = get_random(4, alpha_upper);
  alphas += get_random(4, alpha_lower);
  std::shuffle(alphas.begin(), alphas.end(), rd);

  size_t lower = 0;
  size_t step = password_length / 8;

  for (size_t index = 0; index < 8; index++) {
    std::uniform_int_distribution<int> rand_pos(lower,
                                                ((index + 1) * step) - 1);
    size_t position = rand_pos(rd);
    lower = position + 2;
    pwd[position] = alphas[index];
  }

  // Replace 3 random non-consecutive chars
  // by 3 numeric chars
  std::string numbers = get_random(3, numeric);
  lower = 0;

  step = password_length / 3;
  for (size_t index = 0; index < 3; index++) {
    std::uniform_int_distribution<int> rand_pos(lower,
                                                ((index + 1) * step) - 1);
    size_t position = rand_pos(rd);
    lower = position + 2;
    pwd[position] = numbers[index];
  }

  return pwd;
}

void create_user_with_random_password(
    const IInstance &instance, const std::string &user,
    const std::vector<std::string> &hosts,
    const std::vector<std::tuple<std::string, std::string, bool>> &grants,
    std::string *out_password, bool disable_pwd_expire) {
  assert(!hosts.empty());

  *out_password = generate_password();

  for (int i = 0;; i++) {
    try {
      instance.create_user(user, hosts.front(), *out_password, grants,
                           disable_pwd_expire);
      break;
    } catch (const mysqlshdk::db::Error &e) {
      // If the error is: ERROR 1819 (HY000): Your password does not satisfy
      // the current policy requirements
      // We regenerate the password to retry
      if (e.code() == ER_NOT_VALID_PASSWORD) {
        if (i == kMAX_WEAK_PASSWORD_RETRIES) {
          throw std::runtime_error(
              "Unable to generate a password that complies with active MySQL "
              "server password policies for account " +
              user + "@" + hosts.front());
        }
        *out_password = mysqlshdk::mysql::generate_password();
      } else {
        throw;
      }
    }
  }

  // subsequent user creations shouldn't fail b/c of password
  for (size_t i = 1; i < hosts.size(); i++) {
    instance.create_user(user, hosts[i], *out_password, grants,
                         disable_pwd_expire);
  }
}

void create_user_with_password(
    const IInstance &instance, const std::string &user,
    const std::vector<std::string> &hosts,
    const std::vector<std::tuple<std::string, std::string, bool>> &grants,
    const std::string &password, bool disable_pwd_expire) {
  assert(!hosts.empty());

  for (auto &host : hosts) {
    try {
      instance.create_user(user, host, password, grants, disable_pwd_expire);
    } catch (const mysqlshdk::db::Error &e) {
      // If the error is: ERROR 1819 (HY000): Your password does not satisfy
      // the current policy requirements
      if (e.code() == ER_NOT_VALID_PASSWORD) {
        throw std::runtime_error("Password provided for account " + user + "@" +
                                 host +
                                 " does not comply with active MySQL "
                                 "server password policies");
      } else {
        throw;
      }
    }
  }
}

void set_random_password(const IInstance &instance, const std::string &user,
                         const std::vector<std::string> &hosts,
                         std::string *out_password) {
  assert(!hosts.empty());

  *out_password = generate_password();

  for (int i = 0;; i++) {
    try {
      instance.executef("SET PASSWORD FOR ?@? = /*((*/?/*))*/", user,
                        hosts.front(), *out_password);
      break;
    } catch (const mysqlshdk::db::Error &e) {
      // If the error is: ERROR 1819 (HY000): Your password does not satisfy
      // the current policy requirements
      // We regenerate the password to retry
      if (e.code() == ER_NOT_VALID_PASSWORD) {
        if (i == kMAX_WEAK_PASSWORD_RETRIES) {
          throw std::runtime_error(
              "Unable to generate a password that complies with active MySQL "
              "server password policies for account " +
              user + "@" + hosts.front());
        }
        *out_password = mysqlshdk::mysql::generate_password();
      } else {
        throw;
      }
    }
  }

  // subsequent operations shouldn't fail b/c of password
  for (size_t i = 1; i < hosts.size(); i++) {
    instance.executef("SET PASSWORD FOR ?@? = /*((*/?/*))*/", user, hosts[i],
                      *out_password);
  }
}

namespace schema {
enum class Object_types { Table = 1 << 0, View = 1 << 1 };
using Object_type =
    mysqlshdk::utils::Enum_set<Object_types, Object_types::View>;

std::set<std::string> get_objects(const mysql::IInstance &instance,
                                  const std::string &schema,
                                  schema::Object_type types) {
  // Gets the list of tables to be backed up
  // ---------------------------------------
  std::string query;
  std::shared_ptr<mysqlshdk::db::IResult> result;
  if (types == schema::Object_type::all()) {
    result = instance.queryf(
        "SELECT TABLE_NAME FROM information_schema.tables WHERE "
        "TABLE_SCHEMA = ?",
        schema.c_str());
  } else if (types.is_set(schema::Object_types::Table)) {
    result = instance.queryf(
        "SELECT TABLE_NAME FROM information_schema.tables WHERE "
        "TABLE_SCHEMA = ? AND TABLE_TYPE = 'BASE TABLE'",
        schema.c_str());
  } else if (types.is_set(schema::Object_types::View)) {
    result = instance.queryf(
        "SELECT TABLE_NAME FROM information_schema.tables WHERE "
        "TABLE_SCHEMA = ? AND TABLE_TYPE = 'VIEW'",
        schema.c_str());
  }

  std::set<std::string> objects;
  auto row = result->fetch_one();
  while (row) {
    objects.insert(row->get_string(0));
    row = result->fetch_one();
  }

  return objects;
}

std::set<std::string> get_tables(const mysql::IInstance &instance,
                                 const std::string &schema) {
  return get_objects(instance, schema,
                     schema::Object_type(schema::Object_types::Table));
}

std::set<std::string> get_views(const mysql::IInstance &instance,
                                const std::string &schema) {
  return get_objects(instance, schema,
                     schema::Object_type(schema::Object_types::View));
}

}  // namespace schema

void copy_schema(const mysql::IInstance &instance, const std::string &name,
                 const std::string &target, bool use_existing_schema,
                 bool move_tables) {
  // First creates the target schema
  bool created_schema = false;
  try {
    instance.executef("CREATE SCHEMA !", target.c_str());
    created_schema = true;
  } catch (const mysqlshdk::db::Error &err) {
    if (err.code() == ER_DB_CREATE_EXISTS) {
      if (!use_existing_schema) {
        throw;
      }
    } else {
      throw;
    }
  }

  try {
    // Gets the tables to be copied
    std::map<std::string, std::set<std::string>> src_objects;
    std::map<std::string, std::set<std::string>> tgt_objects;

    instance.execute("SET FOREIGN_KEY_CHECKS=0");

    src_objects["TABLE"] = schema::get_tables(instance, name);
    src_objects["VIEW"] = schema::get_views(instance, name);

    // If the target schema existed, we need to keep track of the objects there
    // to delete the ones that did not exist in the src schema
    if (!created_schema) {
      tgt_objects["TABLE"] = schema::get_tables(instance, target);
      tgt_objects["VIEW"] = schema::get_views(instance, target);
    }

    // If target schema existed, there might be additional objects that are
    // not in the source schema, they need to be deleted
    if (!created_schema) {
      for (const auto &entry : src_objects) {
        std::set<std::string> extra_objects;
        std::set_difference(
            tgt_objects[entry.first].begin(), tgt_objects[entry.first].end(),
            entry.second.begin(), entry.second.end(),
            std::inserter(extra_objects, extra_objects.begin()));

        for (const auto &object : extra_objects) {
          try {
            auto query =
                shcore::sqlstring("DROP " + entry.first + " IF EXISTS !.!",
                                  shcore::QuoteOnlyIfNeeded);
            query << target.c_str() << object.c_str();
            query.done();
            instance.execute(query.str());
          } catch (const mysqlshdk::db::Error &error) {
            // Attempting to drop a previous VIEW that now is a TABLE will give
            // WRONG OBJECT error, however we don't care because it means a VIEW
            // on the target schema got replaced by a TABLE, so the error is
            // bypassed
            if (error.code() != ER_WRONG_OBJECT) throw;
          }
        }
      }
    }

    // Creates the tables on the backup schema
    // ---------------------------------------
    for (const auto &object : src_objects["TABLE"]) {
      // Ensures the table does not exist on the target schema
      if (!created_schema) drop_table_or_view(instance, target, object, true);

      if (move_tables) {
        instance.executef("RENAME TABLE !.! TO !.!", name.c_str(),
                          object.c_str(), target.c_str(), object.c_str());
      } else {
        instance.executef("CREATE TABLE !.! LIKE !.!", target.c_str(),
                          object.c_str(), name.c_str(), object.c_str());
      }
    }

    // Creates the views on the backup schema
    // ---------------------------------------
    if (!src_objects["VIEW"].empty()) {
      std::string current_schema;
      auto result = instance.query("SELECT DATABASE()");
      auto row = result->fetch_one();
      if (!row->is_null(0)) current_schema = row->get_string(0);

      instance.executef("USE !", target.c_str());

      for (const auto &object : src_objects["VIEW"]) {
        // Ensures the target view does not exist on the target schema
        if (!created_schema) drop_view_or_table(instance, target, object, true);

        auto view_result = instance.queryf("SHOW CREATE VIEW !.!", name.c_str(),
                                           object.c_str());

        auto view_row = view_result->fetch_one();

        auto create_stmt = view_row->get_string(1);

        create_stmt = shcore::str_replace(create_stmt, name, target);

        instance.execute(create_stmt);
      }

      if (!current_schema.empty())
        instance.executef("USE !", current_schema.c_str());
    }

    // Backups the table data
    // ----------------------
    if (!move_tables) {
      instance.execute("START TRANSACTION");
      for (const auto &table : src_objects["TABLE"]) {
        // Copies the data from source to target
        instance.executef("INSERT INTO !.! SELECT * FROM !.!", target.c_str(),
                          table.c_str(), name.c_str(), table.c_str());
      }
      instance.execute("COMMIT");
    }
    instance.execute("SET FOREIGN_KEY_CHECKS=1");
  } catch (const std::exception &error) {
    if (!move_tables) instance.execute("ROLLBACK");
    instance.execute("SET FOREIGN_KEY_CHECKS=1");

    if (created_schema) {
      instance.executef("DROP SCHEMA !", target.c_str());
    }
    log_error("Error copying metadata tables: %s", error.what());
    throw;
  }
}

void copy_data(const mysql::IInstance &instance, const std::string &name,
               const std::string &target) {
  bool disable_fk_checks = false;
  try {
    // Gets the tables to be copied
    std::set<std::string> src_objects;
    std::set<std::string> tgt_objects;

    auto result = instance.query("SHOW VARIABLES LIKE 'foreign_key_checks'");
    auto value = result->fetch_one()->get_string(1);
    if (value == "ON") disable_fk_checks = true;

    if (disable_fk_checks) instance.execute("SET FOREIGN_KEY_CHECKS=0");

    src_objects = schema::get_tables(instance, name);
    tgt_objects = schema::get_tables(instance, target);

    // Backups the table data
    // ----------------------
    instance.execute("START TRANSACTION");
    for (const auto &table : src_objects) {
      // Copies the data from source to target
      instance.executef("INSERT INTO !.! SELECT * FROM !.!", target.c_str(),
                        table.c_str(), name.c_str(), table.c_str());
    }
    instance.execute("COMMIT");

    if (disable_fk_checks) instance.execute("SET FOREIGN_KEY_CHECKS=1");
  } catch (const std::exception &error) {
    instance.execute("ROLLBACK");
    if (disable_fk_checks) instance.execute("SET FOREIGN_KEY_CHECKS=1");
    log_error("Error copying data from '%s' to '%s': %s", name.c_str(),
              target.c_str(), error.what());
    throw;
  }
}

/**
 * Creates an indicator/marker at the target instance with the given name.
 *
 * @param instance - where to create the tag
 * @param name - unique name for the tag (<= 64 chars)
 *
 * This marker has the following properties:
 * - persists server restarts
 * - can be executed while super_read_only=1
 * - doesn't generate a transaction
 * - creation is atomic
 * - only requires REPLICATION_SLAVE_ADMIN to be executed
 * - local to the instance only
 */
void create_indicator_tag(const mysql::IInstance &instance,
                          const std::string &name) {
  assert(name.length() <= 64);

  std::string source_term_cmd =
      mysqlshdk::mysql::get_replication_source_keyword(instance.get_version(),
                                                       true);
  std::string source_term =
      mysqlshdk::mysql::get_replication_source_keyword(instance.get_version());

  // The reason this hack is done this way:
  // - any method based on a DB object, requires the user to have privs
  //   to create and drop that DB object
  // - if the DB object is inside a schema the user owns, then that wouldn't
  //   be atomic, since we'd have to create the schema and then the object
  // - doesn't require additional plugins/UDFs
  instance.executef("CHANGE " + source_term_cmd + " TO " + source_term +
                        "_USER=/*(*/ ? /*)*/ FOR CHANNEL ?",
                    mysqlshdk::utils::isotime(), name);
}

void drop_indicator_tag(const mysql::IInstance &instance,
                        const std::string &name) {
  std::string replica_term =
      mysqlshdk::mysql::get_replica_keyword(instance.get_version());

  try {
    instance.executef("RESET " + replica_term + " ALL FOR CHANNEL ?", name);
  } catch (const shcore::Error &e) {
    if (e.code() != ER_SLAVE_CHANNEL_DOES_NOT_EXIST) throw;
  }
}

bool check_indicator_tag(const mysql::IInstance &instance,
                         const std::string &name) {
  if (instance
          .queryf("SELECT * FROM mysql.slave_master_info"
                  " WHERE channel_name = ?",
                  name)
          ->fetch_one())
    return true;
  else
    return false;
}

bool query_server_errors(
    const mysql::IInstance &instance, const std::string &start_time,
    const std::string &end_time, const std::vector<std::string> &subsystems,
    const std::function<void(const Error_log_entry &)> &f) {
  assert(!start_time.empty());

  try {
    std::string query = "SELECT * FROM performance_schema.error_log WHERE";
    query += " logged >= " + shcore::quote_sql_string(start_time);
    if (!end_time.empty())
      query += " and logged <= " + shcore::quote_sql_string(end_time);
    if (!subsystems.empty()) {
      query += " and subsystem in (" +
               shcore::str_join(subsystems, ",",
                                [](const std::string &s) {
                                  return shcore::quote_sql_string(s);
                                }) +
               ")";
    }
    query += " ORDER BY logged";

    auto res = instance.query(query);
    for (auto row = res->fetch_one_named(); row; row = res->fetch_one_named()) {
      Error_log_entry entry;

      entry.logged = row.get_string("LOGGED");
      entry.thread_id = row.get_uint("THREAD_ID");
      entry.prio = row.get_string("PRIO");
      entry.error_code = row.get_string("ERROR_CODE");
      entry.subsystem = row.get_string("SUBSYSTEM");
      entry.data = row.get_string("DATA");

      f(entry);
    }
  } catch (const shcore::Error &e) {
    // error_log table either not supported or pfs disabled or no access to it
    if (e.code() == ER_NO_SUCH_TABLE || e.code() == ER_BAD_DB_ERROR ||
        e.code() == ER_TABLEACCESS_DENIED_ERROR)
      return false;
    throw;
  }

  return true;
}
}  // namespace mysql
}  // namespace mysqlshdk

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

#include "mysqlshdk/libs/mysql/undo.h"
#include <utility>
#include "mysqlshdk/libs/db/utils/utils.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"

namespace mysqlshdk {
namespace mysql {

void Sql_change::execute(const IInstance &instance) {
  for (auto ch = m_changes.rbegin(); ch != m_changes.rend(); ++ch) {
    instance.execute(*ch);
  }
}

std::unique_ptr<Sql_change> Sql_change::create() {
  return std::make_unique<Sql_change>();
}

void Sql_change::add_sql(const std::string &sql) { m_changes.push_back(sql); }

// ----

namespace {

void prepare_delete_undo(
    std::vector<std::string> *changes, const std::string &qtable,
    const std::shared_ptr<mysqlshdk::db::IResult> &result) {
  std::string query = "INSERT INTO " + qtable + " (";
  std::string row_templ;
  for (const auto &col : result->get_metadata()) {
    query += shcore::quote_identifier(col.get_column_name()) + ",";
    row_templ += "?,";
  }
  query.pop_back();  // remove trailing ,
  row_templ.pop_back();
  query += ") VALUES ";

  row_templ = "(" + row_templ + ")";
  while (auto row = result->fetch_one()) {
    shcore::sqlstring sql_row(row_templ, 0);
    for (uint32_t i = 0; i < row->num_fields(); ++i) {
      mysqlshdk::db::feed_field(&sql_row, *row, i);
    }
    query += sql_row.str() + ",";
  }
  query.pop_back();  // remove trailing ,

  changes->emplace_back(std::move(query));
}

void prepare_update_undo(
    std::vector<std::string> *changes, const std::string &qtable,
    const std::string &column, const std::string &where,
    const std::shared_ptr<mysqlshdk::db::IResult> &result) {
  shcore::sqlstring sql("UPDATE " + qtable + " SET " +
                            shcore::quote_identifier(column) + "=? WHERE " +
                            where,
                        0);

  auto row = result->fetch_one_or_throw();
  mysqlshdk::db::feed_field(&sql, *row, 0);

  changes->emplace_back(sql.str());
}
}  // namespace

std::unique_ptr<Transaction_undo> Transaction_undo::create() {
  return std::make_unique<Transaction_undo>();
}

void Transaction_undo::add_snapshot_for_delete(const std::string &qtable,
                                               const IInstance &instance,
                                               const std::string &where) {
  prepare_delete_undo(
      &m_changes, qtable,
      instance.query("SELECT * FROM " + qtable + " WHERE " + where));
}

void Transaction_undo::add_snapshot_for_update(const std::string &qtable,
                                               const std::string &column,
                                               const IInstance &instance,
                                               const std::string &where) {
  prepare_update_undo(
      &m_changes, qtable, column, where,
      instance.query("SELECT " + shcore::quote_identifier(column) + " FROM " +
                     qtable + " WHERE " + where));
}

void Transaction_undo::execute(const IInstance &instance) {
  if (m_changes.size() == 1) {
    Sql_change::execute(instance);
  } else if (!m_changes.empty()) {
    try {
      instance.execute("START TRANSACTION");
      Sql_change::execute(instance);
      instance.execute("COMMIT");
    } catch (...) {
      instance.execute("ROLLBACK");
      throw;
    }
  }
}

// ----

Sql_undo_list::Sql_undo_list(std::unique_ptr<Transaction_undo> trx_undo) {
  m_sql_changes.emplace_back(std::move(trx_undo));
}

void Sql_undo_list::add(std::unique_ptr<Sql_change> change) {
  m_sql_changes.emplace_back(std::move(change));
}

Sql_change *Sql_undo_list::add_sql_change() {
  m_sql_changes.emplace_back(Sql_change::create());
  return m_sql_changes.back().get();
}

void Sql_undo_list::add_snapshot_for_sysvar(const IInstance &instance,
                                            const std::string &variable,
                                            Var_qualifier qualifier) {
  std::string set_stmt_fmt;
  std::string get_stmt;
  if (qualifier == Var_qualifier::GLOBAL) {
    set_stmt_fmt = "SET GLOBAL ! = ?";
    get_stmt = "SELECT @@global." + variable;
  } else if (qualifier == Var_qualifier::PERSIST) {
    set_stmt_fmt = "SET PERSIST ! = ?";
    get_stmt = "SELECT @@global." + variable;
  } else if (qualifier == Var_qualifier::PERSIST_ONLY) {
    set_stmt_fmt = "SET PERSIST_ONLY ! = ?";
    get_stmt = "SELECT @@global." + variable;
  } else {
    set_stmt_fmt = "SET SESSION ! = ?";
    get_stmt = "SELECT @@session." + variable;
  }

  shcore::sqlstring set_stmt = shcore::sqlstring(set_stmt_fmt.c_str(), 0);
  set_stmt << variable;
  mysqlshdk::db::feed_field(&set_stmt,
                            *instance.query(get_stmt)->fetch_one_or_throw(), 0);
  set_stmt.done();

  auto change = add_sql_change();
  change->add_sql(set_stmt.str());
}

void Sql_undo_list::add_snapshot_for_drop_user(const IInstance &instance,
                                               const std::string &user,
                                               const std::string &host) {
  auto change = add_sql_change();

  auto grants = instance.queryf("SHOW GRANTS FOR ?@?", user, host);
  while (auto row = grants->fetch_one()) {
    change->add_sql(row->get_string(0));
  }

  std::string create_user =
      instance.queryf_one_string(0, "", "SHOW CREATE USER ?@?", user, host);
  change->add_sql("CREATE USER IF NOT EXISTS " +
                  create_user.substr(strlen("CREATE USER ")));
}

void Sql_undo_list::execute(const IInstance &instance) {
  for (auto ch = m_sql_changes.rbegin(); ch != m_sql_changes.rend(); ++ch) {
    (*ch)->execute(instance);
  }
}

}  // namespace mysql
}  // namespace mysqlshdk

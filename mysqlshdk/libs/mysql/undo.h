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

#ifndef MYSQLSHDK_LIBS_MYSQL_UNDO_H_
#define MYSQLSHDK_LIBS_MYSQL_UNDO_H_

#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/libs/mysql/instance.h"

namespace mysqlshdk {
namespace mysql {

class Sql_change {
 public:
  static std::unique_ptr<Sql_change> create();

  virtual ~Sql_change() = default;
  virtual void execute(const IInstance &instance);

  void add_sql(const std::string &sql);

 protected:
  std::vector<std::string> m_changes;
};

class Transaction_undo : public Sql_change {
 public:
  static std::unique_ptr<Transaction_undo> create();

  void add_snapshot_for_delete(const std::string &qtable,
                               const IInstance &instance,
                               const std::string &where);

  void add_snapshot_for_update(const std::string &qtable,
                               const std::string &column,
                               const IInstance &instance,
                               const std::string &where);

  void execute(const IInstance &instance) override;
};

/**
 * Manages a list of SQL changes that can be undone.
 *
 * Changes are executed in reverse order of addition.
 */
class Sql_undo_list {
 public:
  Sql_undo_list() = default;
  explicit Sql_undo_list(std::unique_ptr<Transaction_undo> trx_undo);

  void add(std::unique_ptr<Sql_change> change);

  void execute(const IInstance &instance);

  Sql_change *add_sql_change();

  void add_snapshot_for_sysvar(const IInstance &instance,
                               const std::string &variable,
                               mysqlshdk::mysql::Var_qualifier qualifier =
                                   mysqlshdk::mysql::Var_qualifier::GLOBAL);

  void add_snapshot_for_drop_user(const IInstance &instance,
                                  const std::string &user,
                                  const std::string &host);

 private:
  std::vector<std::unique_ptr<Sql_change>> m_sql_changes;
};

}  // namespace mysql
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_MYSQL_UNDO_H_

/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/instance_pool.h"

#include "mysqlshdk/include/shellcore/shell_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

Instance::Instance(const std::shared_ptr<mysqlshdk::db::ISession> &session)
    : mysqlshdk::mysql::Instance(session) {}

std::shared_ptr<mysqlshdk::db::IResult> Instance::query(const std::string &sql,
                                                        bool buffered) const {
  log_sql(sql);
  return mysqlshdk::mysql::Instance::query(sql, buffered);
}

void Instance::execute(const std::string &sql) const {
  log_sql(sql);
  mysqlshdk::mysql::Instance::execute(sql);
}

/**
 * Log the input SQL statement.
 *
 * SQL is logged with the INFO level and according the value set for the
 * dba.logSql shell option:
 *   - 0: no SQL logging;
 *   - 1: log SQL statements, except SELECT and SHOW;
 *   - 2: log all SQL statements;
 * Passwords are hidden, i.e., replaced by ****, assuming that they are
 * properly identified in the SQL statement.
 *
 * @param sql string with the target SQL statement to log.
 *
 */
void Instance::log_sql(const std::string &sql) const {
  if (current_shell_options()->get().dba_log_sql > 1 ||
      (current_shell_options()->get().dba_log_sql == 1 &&
       !shcore::str_ibeginswith(sql, "select") &&
       !shcore::str_ibeginswith(sql, "show"))) {
    log_info(
        "%s: %s", descr().c_str(),
        shcore::str_subvars(
            sql, [](const std::string &) { return "****"; }, "/*((*/", "/*))*/")
            .c_str());
  }
}

}  // namespace dba
}  // namespace mysqlsh

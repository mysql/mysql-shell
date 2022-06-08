/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_SHELLCORE_PROVIDER_SQL_H_
#define MYSQLSHDK_SHELLCORE_PROVIDER_SQL_H_

#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/parser/code-completion/mysql_code_completion_context.h"
#include "shellcore/completer.h"

namespace shcore {
namespace completer {

class Provider_sql : public Provider {
 public:
  Provider_sql();

  ~Provider_sql() override;

  Completion_list complete(const std::string &buffer, const std::string &line,
                           size_t *compl_offset) override;

  void refresh_schema_cache(
      const std::shared_ptr<mysqlshdk::db::ISession> &session);

  void refresh_name_cache(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const std::string &current_schema, bool force);

  void clear_name_cache();

  void interrupt_rehash();

  Completion_list complete_schema(const std::string &prefix) const;

 private:
  class Cache;

  void update_completion_context(
      const std::shared_ptr<mysqlshdk::db::ISession> &session);

  void reset_completion_context();

  mysqlshdk::Sql_completion_context m_completion_context;
  std::unique_ptr<Cache> m_cache;
};

}  // namespace completer
}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_PROVIDER_SQL_H_

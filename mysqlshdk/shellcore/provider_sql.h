/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "shellcore/base_session.h"
#include "shellcore/completer.h"

namespace shcore {
namespace completer {

class Provider_sql : public Provider {
 public:
  Completion_list complete(const std::string &text,
                           size_t *compl_offset) override;

  virtual void refresh_schema_cache(
      std::shared_ptr<mysqlsh::ShellBaseSession> session);

  // void refresh_name_cache(std::shared_ptr<mysqlshdk::db::ISession> session,
  //                         const std::string &current_schema,
  //                         const std::vector<std::string> *table_names,
  //                         bool rehash_all);
  virtual void refresh_name_cache(
      std::shared_ptr<mysqlsh::ShellBaseSession> session,
      const std::string &current_schema,
      const std::vector<std::string> *table_names, bool rehash_all);

  void interrupt_rehash();

  Completion_list complete_schema(const std::string &prefix);

 private:
  std::string default_schema_;
  std::vector<std::string> schema_names_;
  std::vector<std::string> object_names_;
  std::vector<std::string> object_dot_names_;
  bool cancelled_;
};

}  // namespace completer
}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_PROVIDER_SQL_H_

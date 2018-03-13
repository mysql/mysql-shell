/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQL_SECRET_STORE_SECRET_SERVICE_SECRET_SERVICE_HELPER_H_
#define MYSQL_SECRET_STORE_SECRET_SERVICE_SECRET_SERVICE_HELPER_H_

#include <string>
#include <vector>

#include "mysql-secret-store/include/helper.h"

#include "mysql-secret-store/secret-service/secret_tool_invoker.h"

namespace mysql {
namespace secret_store {
namespace secret_service {

class Secret_service_helper : public common::Helper {
 public:
  Secret_service_helper();

  void check_requirements() override;

  void store(const common::Secret &) override;

  void get(const common::Secret_id &, std::string *) override;

  void erase(const common::Secret_id &) override;

  void list(std::vector<common::Secret_id> *) override;

 private:
  Secret_tool_invoker::Attributes get_attributes(const common::Secret_id &id);

  std::string get_label(const common::Secret_id &id);

  std::string get(const Secret_tool_invoker::Attributes &);

  std::string get(const common::Secret_id &id);

  std::string list();

  Secret_tool_invoker m_invoker;
};

}  // namespace secret_service
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_SECRET_SERVICE_SECRET_SERVICE_HELPER_H_

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

#ifndef MYSQL_SECRET_STORE_CORE_COMMAND_H_
#define MYSQL_SECRET_STORE_CORE_COMMAND_H_

#include <istream>
#include <ostream>
#include <string>

#include "mysql-secret-store/include/helper.h"

namespace mysql {
namespace secret_store {
namespace core {

class Command {
 public:
  Command(const std::string &name, common::Helper *helper)
      : m_name{name}, m_helper{helper} {}
  virtual ~Command() = default;

  Command(const Command &) = delete;
  Command(Command &&) = delete;
  Command &operator=(const Command &) = delete;
  Command &operator=(Command &&) = delete;

  const std::string &name() const noexcept { return m_name; }

  virtual std::string help() const = 0;

  virtual void execute(std::istream *input, std::ostream *output) = 0;

 protected:
  common::Helper *helper() const noexcept { return m_helper; }

 private:
  std::string m_name;
  common::Helper *m_helper;
};

}  // namespace core
}  // namespace secret_store
}  // namespace mysql

#endif  // MYSQL_SECRET_STORE_CORE_COMMAND_H_

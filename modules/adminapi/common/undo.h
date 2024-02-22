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

#ifndef MODULES_ADMINAPI_COMMON_UNDO_H_
#define MODULES_ADMINAPI_COMMON_UNDO_H_

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include "modules/adminapi/common/instance_pool.h"
#include "mysqlshdk/libs/mysql/undo.h"

namespace mysqlsh {
namespace dba {

using mysqlshdk::mysql::Sql_change;
using mysqlshdk::mysql::Sql_undo_list;
using mysqlshdk::mysql::Transaction_undo;

class Undo_tracker {
 public:
  class Undo_entry {
   public:
    virtual ~Undo_entry() = default;
    virtual bool call() = 0;
    virtual void cancel() = 0;
  };

  Undo_entry &add(
      const std::string &note, mysqlshdk::mysql::Sql_undo_list sql_undo,
      const std::function<std::shared_ptr<Instance>()> &get_instance);
  Undo_entry &add(const std::string &note, const std::function<void()> &f);

  void execute();

 private:
  std::list<std::pair<std::string, std::unique_ptr<Undo_entry>>> m_entries;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_UNDO_H_

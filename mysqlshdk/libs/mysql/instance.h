/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_
#define MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_

#include <map>
#include <string>
#include <vector>

#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/nullable.h"


namespace mysqlshdk {
namespace mysql {

/**
 * This interface defines the low level instance operations to be available.
 */
class IInstance {
  virtual utils::nullable<bool> get_sysvar_bool(
      const std::string& name) const = 0;
  virtual utils::nullable<std::string> get_sysvar_string(
      const std::string& name) const = 0;
  virtual utils::nullable<int64_t> get_sysvar_int(
      const std::string& name) const = 0;
  virtual std::shared_ptr<db::ISession> get_session() const = 0;
};

/**
 * Implementation of the low level instance operations.
 */
class Instance : public IInstance {
 public:
  explicit Instance(std::shared_ptr<db::ISession> session);

  virtual utils::nullable<bool> get_sysvar_bool(const std::string& name) const;
  virtual utils::nullable<std::string> get_sysvar_string(
      const std::string& name) const;
  virtual utils::nullable<int64_t> get_sysvar_int(
      const std::string& name) const;
  virtual std::shared_ptr<db::ISession> get_session() const {return _session;}
  std::map<std::string, utils::nullable<std::string> > get_system_variables(
      const std::vector<std::string>& names) const;

 private:
  std::shared_ptr<db::ISession> _session;
};

}  // namespace mysql
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_MYSQL_INSTANCE_H_

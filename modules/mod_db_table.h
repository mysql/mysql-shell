/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

// Interactive Table access module
// (the one exposed as the table members of the db object in the shell)

#ifndef _MOD_DB_TABLE_H_
#define _MOD_DB_TABLE_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include "mod_mysql.h"

#include <boost/weak_ptr.hpp>

namespace shcore
{
  class Shell_core;
};

namespace mysh {

class Db;

class Db_table : public shcore::Cpp_object_bridge
{
public:
  Db_table(boost::shared_ptr<Db> owner, const std::string &name);
  ~Db_table();

  virtual std::string &append_descr(std::string &s_out, int indent, int quote_strings) const;

  virtual std::string class_name() const;
  virtual std::vector<std::string> get_members() const;
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual bool operator == (const Object_bridge &other) const;
private:
  boost::weak_ptr<Db> _owner;
  std::string _table_name;
};

};

#endif

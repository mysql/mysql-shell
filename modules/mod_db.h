/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef _MOD_DB_H_
#define _MOD_DB_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>

namespace shcore
{
  class Proxy_object;
};

namespace mysh {

class Session;
class Db_table;

class MOD_PUBLIC Db : public shcore::Cpp_object_bridge, public boost::enable_shared_from_this<Db>
{
public:
  Db(boost::shared_ptr<Session> owner, const std::string &name);
  ~Db();

  virtual std::string class_name() const;
  virtual std::string &append_descr(std::string &s_out, int indent=-1, int quote_strings=0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual std::vector<std::string> get_members() const;
  virtual bool operator == (const Object_bridge &other) const;
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void set_member(const std::string &prop, shcore::Value value);

  void cache_table_names();

  boost::weak_ptr<Session> session() const { return _session; }

  std::string schema() const { return _schema; }
private:
  boost::weak_ptr<Session> _session;
  boost::shared_ptr<shcore::Value::Map_type> _tables;
  boost::shared_ptr<shcore::Value::Map_type> _collections;

  std::string _schema;
};

};

#endif

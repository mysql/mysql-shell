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

// Interactive session access module
// Exposed as "session" in the shell

#ifndef _MOD_SESSION_H_
#define _MOD_SESSION_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include "mod_connection.h"

#include <boost/enable_shared_from_this.hpp>

namespace shcore
{
  class Shell_core;
  class Proxy_object;
};

namespace mysh {

class Db;


class Session : public shcore::Cpp_object_bridge, public boost::enable_shared_from_this<Session>
{
public:
  Session(shcore::Shell_core *shc);
  ~Session();

  virtual std::string class_name() const;
  virtual std::string &append_descr(std::string &s_out, int indent=-1, int quote_strings=0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual std::vector<std::string> get_members() const;
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual bool operator == (const Object_bridge &other) const;

  shcore::Value connect(const shcore::Argument_list &args);
  void disconnect() { _conn.reset();  }
  shcore::Value sql(const shcore::Argument_list &args);
  shcore::Value sql_one(const shcore::Argument_list &args);

  void print_exception(const shcore::Exception &e);

  boost::shared_ptr<Base_connection> conn() const { return _conn; }

  boost::shared_ptr<Db> default_schema();

  bool is_connected() { return _conn ? true : false;  }

private:
  shcore::Value get_db(const std::string &schema);

  shcore::Shell_core *_shcore;
  boost::shared_ptr<Base_connection> _conn;

  boost::shared_ptr<shcore::Proxy_object> _schema_proxy;

  bool _show_warnings;
};

};

#endif

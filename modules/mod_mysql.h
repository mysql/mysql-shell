/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef _MOD_MYSQL_H_
#define _MOD_MYSQL_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include <mysql.h>

namespace mysh {

class Mysql_resultset;

class Mysql_connection : public shcore::Cpp_object_bridge
{
public:
  Mysql_connection(const std::string &uri, const std::string &password="");
  ~Mysql_connection();

  virtual std::string class_name() const;
  virtual std::string &append_descr(std::string &s_out, int indent=-1, int quote_strings=0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual std::vector<std::string> get_members() const;
  virtual bool operator == (const Object_bridge &other) const;
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void set_member(const std::string &prop, shcore::Value value);

  shcore::Value close(const shcore::Argument_list &args);
  shcore::Value sql(const shcore::Argument_list &args);
//  shcore::Value stats(const shcore::Argument_list &args);

  static boost::shared_ptr<Object_bridge> create(const shcore::Argument_list &args);
public:
  std::string uri() const { return _uri; }

public:
  MYSQL_RES *raw_sql(const std::string &sql);
  MYSQL_RES *next_result();
  my_ulonglong affected_rows();
  unsigned int warning_count();
  const char *get_info();
private:
  std::string _uri;
  MYSQL *_mysql;
};

};

#endif

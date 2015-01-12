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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef _MOD_DB_H_
#define _MOD_DB_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

#include "mod_mysql.h"

namespace shcore
{
  class Shell_core;
};

namespace mysh {

class Mysql_resultset;

class Db : public shcore::Cpp_object_bridge
{
public:
  Db(shcore::Shell_core *shc);
  ~Db();

  virtual std::string class_name() const;
  virtual std::string &append_descr(std::string &s_out, int indent=-1, int quote_strings=0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual std::vector<std::string> get_members() const;
  virtual bool operator == (const Object_bridge &other) const;
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual void set_member(const std::string &prop, shcore::Value value);

  shcore::Value connect(const shcore::Argument_list &args);
  shcore::Value connect_add(const shcore::Argument_list &args);
  shcore::Value sql(const shcore::Argument_list &args);
  
  void internal_sql(boost::shared_ptr<Mysql_connection> conn, const std::string& sql, boost::function<void (MYSQL_RES* data)> result_handler);
//  shcore::Value stats(const shcore::Argument_list &args);

private:
  void print_exception(const shcore::Exception &e);
  void print_warnings(MYSQL_RES* data, unsigned long main_error_code);
  void print_result(MYSQL_RES *res);
  void print_table(MYSQL_RES *res);
  void print_json(MYSQL_RES *res);
  void print_vertical(MYSQL_RES *res);

  shcore::Shell_core *_shcore;
  std::vector<boost::shared_ptr<Mysql_connection> > _conns;
};

};

#endif

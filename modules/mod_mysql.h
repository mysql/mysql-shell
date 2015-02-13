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

#if WIN32
#  include <winsock2.h>
#endif
#include <mysql.h>

namespace mysh {

class Mysql_resultset : public shcore::Cpp_object_bridge
{
public:
  Mysql_resultset(MYSQL_RES *res, unsigned long raw_duration = 0, my_ulonglong affected_rows = 0, unsigned int warning_count = 0, const char *info = NULL, boost::shared_ptr<shcore::Value::Map_type> options = boost::shared_ptr<shcore::Value::Map_type>());
  virtual ~Mysql_resultset();

  virtual std::string class_name() const
  {
    return "mysql_resultset";
  }

  virtual std::vector<std::string> get_members() const;
  virtual bool operator == (const Object_bridge &other) const;
  virtual shcore::Value get_member(const std::string &prop) const;
  shcore::Value next(const shcore::Argument_list &args);
  shcore::Value print(const shcore::Argument_list &args);

  int warning_count() { return _warning_count; }


private:
  void print_result();
  void print_table();
  shcore::Value row_to_doc(const MYSQL_ROW &row, unsigned long *lengths);

private:
  MYSQL_RES *_result;
  MYSQL_FIELD *_fields;
  int _num_fields;
  unsigned long _raw_duration;
  my_ulonglong _affected_rows;
  std::string _info;

  int _warning_count;
  
  bool _key_by_index;
};



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
  shcore::Value sql_(const shcore::Argument_list &args);
  shcore::Value sql(const std::string &sql, shcore::Value options);
//  shcore::Value stats(const shcore::Argument_list &args);

  shcore::Value sql_one(const std::string &sql);
  shcore::Value sql_one_(const shcore::Argument_list &args);

  static boost::shared_ptr<Object_bridge> create(const shcore::Argument_list &args);
public:
  std::string uri() const { return _uri; }

public:
  MYSQL_RES *raw_sql(const std::string &sql);
  MYSQL_RES *next_result();
private:
  std::string _uri;
  MYSQL *_mysql;
};

};

#endif

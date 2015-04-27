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
#include "mod_connection.h"

namespace mysh {

class Mysql_row : public Base_row
{
public:
  Mysql_row(std::vector<Field>& metadata, bool key_by_index, MYSQL_ROW row, unsigned long *lengths);
  virtual ~Mysql_row() {}

  virtual shcore::Value get_value(int index);
  virtual std::string get_value_as_string(int index);

private:
  MYSQL_ROW _row;
  unsigned long *_lengths;
};

class Mysql_connection;
class Mysql_resultset : public Base_resultset
{
public:
  Mysql_resultset(boost::shared_ptr<Mysql_connection> owner, uint64_t affected_rows, int warning_count, const char *info, boost::shared_ptr<shcore::Value::Map_type> options = boost::shared_ptr<shcore::Value::Map_type>());
  virtual ~Mysql_resultset();

  void reset(boost::shared_ptr<MYSQL_RES> res, unsigned long duration);

  virtual std::string class_name() const { return "Mysql_resultset"; }
  virtual int fetch_metadata();

protected:
  virtual Base_row* next_row();

private:
  boost::weak_ptr<MYSQL_RES> _result;
};


class Mysql_connection : public Base_connection, public boost::enable_shared_from_this<Mysql_connection>
{
public:
  Mysql_connection(const std::string &uri, const char *password = NULL);
  ~Mysql_connection();

  virtual std::string class_name() const { return "Mysql_connection"; }

  virtual shcore::Value close(const shcore::Argument_list &args);
  virtual shcore::Value sql(const std::string &sql, shcore::Value options);
  virtual shcore::Value sql_one(const std::string &sql);

  virtual bool next_result(Base_resultset *target, bool first_result = false);

  static boost::shared_ptr<Object_bridge> create(const shcore::Argument_list &args);

private:
  std::string _uri;
  MYSQL *_mysql;

  boost::shared_ptr<MYSQL_RES> _prev_result;
  virtual Mysql_resultset *_sql(const std::string &sql, shcore::Value options);
};

};

#endif

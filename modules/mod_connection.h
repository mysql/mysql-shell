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

#ifndef _MOD_CONNECTION_H_
#define _MOD_CONNECTION_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "../utils/utils_time.h"

#include <boost/enable_shared_from_this.hpp>

namespace shcore
{
  class Shell_core;
  class Proxy_object;
};

namespace mysh {

bool parse_mysql_connstring(const std::string &connstring,
                                   std::string &protocol, std::string &user, std::string &password,
                                   std::string &host, int &port, std::string &sock,
                                   std::string &db, int &pwd_found);
std::string strip_password(const std::string &connstring);
class Base_resultset;
class Base_connection : public shcore::Cpp_object_bridge
{
public:
  Base_connection(const std::string &uri, const char *password = NULL);

  // Methods from Cpp_object_bridge will be defined here
  // Since all the connections will expose the same members
  virtual bool operator == (const Object_bridge &other) const;

  virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;

  virtual std::vector<std::string> get_members() const;
  virtual shcore::Value get_member(const std::string &prop) const;

  // set_member should be implemented in case there are write properties
  //virtual void set_member(const std::string &prop, shcore::Value value);

  virtual shcore::Value close(const shcore::Argument_list &args) = 0;
  virtual shcore::Value sql(const std::string &sql, shcore::Value options) = 0;
  virtual shcore::Value sql_one(const std::string &sql) = 0;
  virtual bool next_result(Base_resultset *target, bool first_result = false) = 0;

  virtual shcore::Value sql_(const shcore::Argument_list &args);
  virtual shcore::Value sql_one_(const shcore::Argument_list &args);

  std::string uri() const { return _uri; }

protected:
  MySQL_timer _timer;
private:
  std::string _uri;
  std::string _pwd;
};

class Field
{
public:
  Field(const std::string& catalog, const std::string& db, const std::string& table, const std::string& otable, const std::string& name, const std::string& oname, int length, int type, int flags, int decimals, int charset);

  const std::string& catalog() { return _catalog; };
  const std::string& db() { return _db; };
  const std::string& table() { return _table; };
  const std::string& org_table() { return _org_table; };
  const std::string& name() { return _name; };
  const std::string& org_name() { return _org_name; };
  const unsigned int length() { return _length; }
  const unsigned int type() { return _type; }
  const unsigned int flags() { return _flags; }
  const unsigned int decimals() { return _decimals; }
  const unsigned int charset() { return _charset; }

  const unsigned int max_length() { return _max_length; }
  void max_length(int length) { _max_length = length; }

  const unsigned int name_length() { return _name_length; }

private:
  std::string _catalog;
  std::string _db;
  std::string _table;
  std::string _org_table;
  std::string _name;
  std::string _org_name;
  unsigned long _length;
  unsigned int _type;
  unsigned int _flags;
  unsigned int _decimals;
  unsigned int _charset;

  unsigned long _max_length;
  unsigned long _name_length;
};

class Base_row
{
public:
  Base_row(std::vector<Field>& metadata, bool key_by_index) :_key_by_index(key_by_index), _fields(metadata){}
  virtual ~Base_row(){}
  virtual shcore::Value get_value(int index) = 0;
  virtual std::string get_value_as_string(int index) = 0;
  shcore::Value as_document();
  shcore::Value as_data_array();

protected:
  bool _key_by_index;
  std::vector<Field>& _fields;
};

class Base_resultset : public shcore::Cpp_object_bridge
{
public:
  Base_resultset(uint64_t affected_rows, int warning_count, const char* info, boost::shared_ptr<shcore::Value::Map_type> options = boost::shared_ptr<shcore::Value::Map_type>());

  // Methods from Cpp_object_bridge will be defined here
  // Since all the connections will expose the same members
  virtual std::vector<std::string> get_members() const;
  virtual shcore::Value get_member(const std::string &prop) const;
  virtual bool operator == (const Object_bridge &other) const;

  // Retrieves the metadata for an executed statement
  // Returns -1 in case of error or the number of records loaded
  virtual int fetch_metadata() = 0;

  // Business logic methods common to all connections
  bool has_rows();
  bool has_resultset() { return _has_resultset; };
  int warning_count() { return _warning_count; }
  shcore::Value print(const shcore::Argument_list &args);
  shcore::Value next(const shcore::Argument_list &args);
  shcore::Value next_result(const shcore::Argument_list &args);
  shcore::Value get_metadata(const shcore::Argument_list &args);
  shcore::Value fetch_all(const shcore::Argument_list &args);

private:
  void print_result(shcore::Value::Array_type_ref records);
  void print_table(shcore::Value::Array_type_ref records);

protected:
  // Methods requiring database interaction, must be
  // defined by the subclasses
  virtual Base_row *next_row() = 0;
  virtual bool next_result() = 0;


  bool _key_by_index;
  std::vector<Field> _metadata;

  bool _has_resultset;  // Indicates active result is result from a query statement
  int _fetched_row_count;       // Returns the number of read records

  unsigned long _raw_duration;
  uint64_t _affected_rows;
  std::string _info;
  int _warning_count;
};

};

#endif

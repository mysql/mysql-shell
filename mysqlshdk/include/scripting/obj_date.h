/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_OBJ_DATE_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_OBJ_DATE_H_

#include <mysqlxclient/xdatetime.h>
#include <string>

#include "mysqlshdk/include/scripting/types_cpp.h"

namespace mysqlx {
class DateTime;
}

namespace shcore {
class SHCORE_PUBLIC Date : public Cpp_object_bridge {
 public:
  Date(int year, int month, int day, int hour, int min, int sec, int usec);
  Date(int year, int month, int day);

  virtual std::string class_name() const { return "Date"; }

  virtual std::string &append_descr(std::string &s_out, int indent = -1,
                                    int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  virtual Value get_member(const std::string &prop) const;
  virtual void set_member(const std::string &prop, Value value);

  virtual bool operator==(const Object_bridge &other) const;
  bool operator==(const Date &other) const;

  int64_t as_ms() const;

  int get_year() const { return _year; }
  int get_month() const { return _month + 1; }
  int get_day() const { return _day; }
  int get_hour() const { return _hour; }
  int get_min() const { return _min; }
  int get_sec() const { return _sec; }
  int get_usec() const { return _usec; }

  bool has_time() const { return _has_time; }

 public:
  static Object_bridge_ref unrepr(const std::string &s);
  static Object_bridge_ref from_ms(int64_t ms_since_epoch);
  static Object_bridge_ref from_mysqlx_datetime(const ::xcl::DateTime &dt);

 private:
  Date(int year, int month, int day, int hour, int min, int sec, int usec,
       bool has_time);

  void validate();

  int _year;
  int _month;
  int _day;
  int _hour;
  int _min;
  int _sec;
  int _usec;
  bool _has_time;
};
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_OBJ_DATE_H_

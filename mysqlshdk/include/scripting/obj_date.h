/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/types_cpp.h"
#include <mysqlxclient/xdatetime.h>

#ifndef _SHCORE_OBJ_DATE_H_
#define _SHCORE_OBJ_DATE_H_

namespace mysqlx {
class DateTime;
}

namespace shcore {
class SHCORE_PUBLIC Date : public Cpp_object_bridge {
public:
  Date(int year, int month, int day, int hour, int min, double sec);
  Date(int year, int month, int day);

  virtual std::string class_name() const { return "Date"; }

  virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual void append_json(shcore::JSON_dumper& dumper) const;

  virtual Value get_member(const std::string &prop) const;
  virtual void set_member(const std::string &prop, Value value);

  virtual bool operator == (const Object_bridge &other) const;
  bool operator == (const Date &other) const;

  int64_t as_ms() const;

  int get_year() const { return _year; }
  int get_month() const { return _month; }
  int get_day() const { return _day; }
  int get_hour() const { return _hour; }
  int get_min() const { return _min; }
  double get_sec() const { return _sec; }

public:
  static Object_bridge_ref create(const shcore::Argument_list &args);
  static Object_bridge_ref unrepr(const std::string &s);
  static Object_bridge_ref from_ms(int64_t ms_since_epoch);
  static Object_bridge_ref from_mysqlx_datetime(const ::xcl::DateTime &dt);

private:
  int _year;
  int _month;
  int _day;
  int _hour;
  int _min;
  double _sec;
  bool _has_time;
};
}

#endif

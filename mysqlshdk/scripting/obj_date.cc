/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/obj_date.h"

#include <time.h>
#include <cstdio>

#include "scripting/common.h"
#include "utils/utils_string.h"
#include "utils/utils_json.h"
#include "scripting/object_factory.h"

using namespace shcore;

Date::Date(int year, int month, int day, int hour, int min, double sec)
    : _year(year),
      _month(month),
      _day(day),
      _hour(hour),
      _min(min),
      _sec(sec),
      _has_time(true) {
}

Date::Date(int year, int month, int day)
    : _year(year),
      _month(month),
      _day(day),
      _hour(0),
      _min(0),
      _sec(0),
      _has_time(false) {
}

bool Date::operator == (const Object_bridge &other) const {
  if (other.class_name() == "Date") {
    return *this == *static_cast<const Date *>(&other);
  }
  return false;
}

bool Date::operator == (const Date &other) const {
  if (_year == other._year &&
      _month == other._month &&
      _day == other._day &&
      _hour == other._hour &&
      _min == other._min &&
      _sec == other._sec &&
      _has_time == other._has_time)
    return true;
  return false;
}

std::string &Date::append_descr(std::string &s_out, int /*indent*/,
                                int quote_strings) const {
  if (quote_strings)
    s_out.push_back(quote_strings);

  if (_has_time) {
    if (static_cast<double>(static_cast<int>(_sec)) != _sec)
      s_out.append(str_format("%04d-%02d-%02d %02d:%02d:%09.6f", _year,
                              (_month + 1), _day, _hour, _min, _sec));
    else
      s_out.append(str_format("%04d-%02d-%02d %02d:%02d:%02d", _year,
                              (_month + 1), _day, _hour, _min,
                              static_cast<int>(_sec)));
  } else {
    s_out.append(str_format("%04d-%02d-%02d", _year, (_month + 1), _day));
  }
  if (quote_strings)
    s_out.push_back(quote_strings);
  return s_out;
}

std::string &Date::append_repr(std::string &s_out) const {
  return append_descr(s_out, 0, '"');
}

void Date::append_json(shcore::JSON_dumper& dumper) const {
  dumper.start_object();

  dumper.append_int("year", _year);
  dumper.append_int("month", _month);
  dumper.append_int("day", _day);
  dumper.append_int("hour", _hour);
  dumper.append_int("minute", _min);
  dumper.append_float("second", _sec);

  dumper.end_object();
}

Value Date::get_member(const std::string &prop) const {
  return Cpp_object_bridge::get_member(prop);
}

void Date::set_member(const std::string &prop, Value value) {
  Cpp_object_bridge::set_member(prop, value);
}

Object_bridge_ref Date::create(const shcore::Argument_list &args) {
  args.ensure_count(3, 6, "Date()");

#define GETi(i) (args.size() > i ? args.int_at(i) : 0)
#define GETf(i) (args.size() > i ? args.double_at(i) : 0)

  if (args.size() == 3)
    return Object_bridge_ref(new Date(GETi(0), GETi(1), GETi(2)));
  else
    return Object_bridge_ref(
        new Date(GETi(0), GETi(1), GETi(2), GETi(3), GETi(4), GETf(5)));
#undef GETi
#undef GETf
}

Object_bridge_ref Date::unrepr(const std::string &s) {
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int min = 0;
  double sec = 0.0;

  int c = sscanf(s.c_str(), "%d-%d-%d %d:%d:%lf", &year, &month, &day, &hour,
                 &min, &sec);
  if (c == 3)
    return Object_bridge_ref(new Date(year, month - 1, day));
  else if (c == 6)
    return Object_bridge_ref(new Date(year, month - 1, day, hour, min, sec));
  else
    throw std::invalid_argument("Invalid date value '" + s + "'");
}

int64_t Date::as_ms() const {
  struct tm t;
  // caution, this obviously doesnt work for dates before 1970 yr
  t.tm_year = _year - 1900;
  t.tm_mon = _month;
  t.tm_mday = _day;
  t.tm_hour = _hour;
  t.tm_min = _min;
  t.tm_sec = static_cast<int>(_sec);

  int64_t seconds_since_epoch = mktime(&t);

  return seconds_since_epoch * 1000 + static_cast<int>(_sec * 1000.0) % 1000;
}

Object_bridge_ref Date::from_ms(int64_t ms_since_epoch) {
  double ms = ms_since_epoch % 1000;
  time_t seconds_since_epoch = ms_since_epoch / 1000;

  struct tm t;
#if WIN32
  localtime_s(&t, &seconds_since_epoch);
#else
  localtime_r(&seconds_since_epoch, &t);
#endif

  return Object_bridge_ref(new Date(t.tm_year + 1900, t.tm_mon, t.tm_mday,
                                    t.tm_hour, t.tm_min,
                                    t.tm_sec + ms / 1000.0));
}

Object_bridge_ref Date::from_mysqlx_datetime(const xcl::DateTime &date) {
  return Object_bridge_ref(new Date(
      date.year(), date.month()-1, date.day(), date.hour(), date.minutes(),
      date.seconds() + (static_cast<double>(date.useconds()) / 1000000)));
}

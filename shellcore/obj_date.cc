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

#include "shellcore/obj_date.h"
#include "shellcore/common.h"

#include <boost/format.hpp>
#include <cstdio>

using namespace shcore;

#include "shellcore/object_factory.h"
REGISTER_OBJECT(mysqlx, Date);

Date::Date(int year, int month, int day, int hour, int min, float sec)
: _year(year), _month(month), _day(day), _hour(hour), _min(min), _sec(sec)
{
}

bool Date::operator == (const Object_bridge &other) const
{
  if (other.class_name() == "Date")
  {
    return *this == *(Date*)&other;
  }
  return false;
}

bool Date::operator == (const Date &other) const
{
  if (_year == other._year &&
      _month == other._month &&
      _day == other._day &&
      _hour == other._hour &&
      _min == other._min &&
      _sec == other._sec)
    return true;
  return false;
}

std::string &Date::append_descr(std::string &s_out, int UNUSED(indent), int quote_strings) const
{
  if (quote_strings)
    s_out.push_back((char)quote_strings);
  if ((float)(int)_sec != _sec)
    s_out.append((boost::format("%04i-%02i-%02i %i:%02i:%02.3f") % _year % (_month + 1) % _day % _hour % _min % _sec).str());
  else
    s_out.append((boost::format("%1$04d-%2$02i-%3$02i %4$i:%5$02i:%6$02i") % _year % (_month + 1) % _day % _hour % _min % (int)_sec).str());
  if (quote_strings)
    s_out.push_back((char)quote_strings);
  return s_out;
}

std::string &Date::append_repr(std::string &s_out) const
{
  return append_descr(s_out, 0, '"');
}

void Date::append_json(shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  dumper.append_int("year", _year);
  dumper.append_int("month", _month);
  dumper.append_int("day", _day);
  dumper.append_int("hour", _hour);
  dumper.append_int("minute", _min);
  dumper.append_float("second", _sec);

  dumper.end_object();
}

Value Date::get_member(const std::string &prop) const
{
  return Cpp_object_bridge::get_member(prop);
}

void Date::set_member(const std::string &prop, Value value)
{
  Cpp_object_bridge::set_member(prop, value);
}

Object_bridge_ref Date::create(const shcore::Argument_list &args)
{
  args.ensure_count(3, 6, "Date()");

#define GETi(i) (args.size() > i ? args.int_at(i) : 0)
#define GETf(i) (args.size() > i ? args.double_at(i) : 0)

  return Object_bridge_ref(new Date(GETi(0), GETi(1), GETi(2), GETi(3), GETi(4), GETf(5)));

#undef GETi
#undef GETf
}

Object_bridge_ref Date::unrepr(const std::string &s)
{
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int min = 0;
  float sec = 0.0;

  sscanf(s.c_str(), "%i-%i-%i %i:%i:%f", &year, &month, &day, &hour, &min, &sec);
  return Object_bridge_ref(new Date(year, month - 1, day, hour, min, sec));
}

int64_t Date::as_ms() const
{
  struct tm t;
  // caution, this obviously doesnt work for dates before 1970 yr
  t.tm_year = _year - 1900;
  t.tm_mon = _month;
  t.tm_mday = _day;
  t.tm_hour = _hour;
  t.tm_min = _min;
  t.tm_sec = (int)_sec;

  int64_t seconds_since_epoch = mktime(&t);

  return seconds_since_epoch * 1000 + (int)(_sec * 1000.0) % 1000;
}

Object_bridge_ref Date::from_ms(int64_t ms_since_epoch)
{
  int ms = ms_since_epoch % 1000;
  time_t seconds_since_epoch = ms_since_epoch / 1000;

  struct tm t;
#if WIN32
  localtime_s(&t, &seconds_since_epoch);
#else
  localtime_r(&seconds_since_epoch, &t);
#endif

  return Object_bridge_ref(new Date(t.tm_year + 1900, t.tm_mon, t.tm_mday,
                                    t.tm_hour, t.tm_min, t.tm_sec + (float)ms / 1000.0));
}
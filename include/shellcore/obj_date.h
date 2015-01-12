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

#include "shellcore/types_cpp.h"

#ifndef _SHCORE_OBJ_DATE_H_
#define _SHCORE_OBJ_DATE_H_

namespace shcore
{
  class Date : public Cpp_object_bridge
  {
  public:
    Date(int year, int month, int day, int hour, int min, float sec);

    virtual std::string class_name() const { return "Date"; }

    virtual std::string &append_descr(std::string &s_out, int indent=-1, int quote_strings=0) const;
    virtual std::string &append_repr(std::string &s_out) const;

    virtual std::vector<std::string> get_members() const;
    virtual Value get_member(const std::string &prop) const;
    virtual void set_member(const std::string &prop, Value value);

    virtual bool operator == (const Object_bridge &other) const;
    bool operator == (const Date &other) const;

    int64_t as_ms() const;

  public:
    static Object_bridge_ref create(const shcore::Argument_list &args);
    static Object_bridge_ref unrepr(const std::string &s);
    static Object_bridge_ref from_ms(int64_t ms_since_epoch);

  private:
    int _year;
    int _month;
    int _day;
    int _hour;
    int _min;
    float _sec;
  };

}

#endif

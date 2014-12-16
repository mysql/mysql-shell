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

#include "shellcore/core_types.h"

using namespace shcore;

static std::map<std::string, Value (*)(const std::string&)> Native_object_factory;


Value Native_object::construct(const std::string &type_name)
{
}

Value Native_object::reconstruct(const std::string &repr)
{

}

void Native_object::register_native_type(const std::string &type_name,
                                         Value (*factory)(const std::string&))
{

}



Value::~Value()
{
  switch (type)
  {
    case Null:
      break;
    case String:
      delete value.s;
      break;
  }
}



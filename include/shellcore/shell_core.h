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

#ifndef _SHELLCORE_H_
#define _SHELLCORE_H_

#include "shellcore/core_types.h"

namespace shcore {


class Registry
{
public:

private:
  Value::Ref _connections;
};


class Language_context
{
public:
};


class Shell_core
{
public:
  Shell_core();

private:
  Value::Ref get_global(const std::string &name);
  bool set_global(const std::string &name, Value::Ref value);


private:
  Registry _registry;
  std::map<std::string, Language_context*> _langs;
};

};

#endif // _SHELLCORE_H_


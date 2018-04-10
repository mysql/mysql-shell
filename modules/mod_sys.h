/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
#include "shellcore/ishell_core.h"

#ifndef _MODULES_MOD_SYS_H_
#define _MODULES_MOD_SYS_H_

namespace mysqlsh {
/**
 * \ingroup ShellAPI
 * $(SYS_BRIEF)
 */
class SHCORE_PUBLIC Sys : public shcore::Cpp_object_bridge {
 public:
  Sys(shcore::IShell_core *owner);
  virtual ~Sys();

  virtual std::string class_name() const { return "Sys"; };
  virtual bool operator==(const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

#if DOXYGEN_JS
  Array path;
  Array argv;
#endif

 protected:
  void init();

  shcore::Value::Array_type_ref _argv;
  shcore::Value::Array_type_ref _path;
};
}  // namespace mysqlsh

#endif

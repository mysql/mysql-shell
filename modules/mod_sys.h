/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
    Sys(shcore::IShell_core* owner);
    virtual ~Sys();

    virtual std::string class_name() const { return "Sys"; };
    virtual bool operator == (const Object_bridge &other) const;

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
}

#endif

/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

// Interactive DB access module
// (the one exposed as the db variable in the shell)

#ifndef _INTERACTIVE_DBA_FARM_H_
#define _INTERACTIVE_DBA_FARM_H_

#include "interactive_object_wrapper.h"

namespace shcore
{
  class SHCORE_PUBLIC Interactive_dba_farm : public Interactive_object_wrapper
  {
  public:
    Interactive_dba_farm(Shell_core& shell_core) : Interactive_object_wrapper("dba", shell_core){ init(); }

    void init();

    shcore::Value add_seed_instance(const shcore::Argument_list &args);
    shcore::Value add_instance(const shcore::Argument_list &args);
    shcore::Value get_farm(const shcore::Argument_list &args) const;

  private:
    bool resolve_instance_options(const std::string& function, const shcore::Argument_list &args, shcore::Value::Map_type_ref &options) const;
  };
}

#endif // _INTERACTIVE_DBA_FARM_H_

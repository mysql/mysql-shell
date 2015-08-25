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

// Interactive Expression access module
// Exposed as "Expression" in the shell

#ifndef _MOD_MYSQLX_EXPRESSION_H_
#define _MOD_MYSQLX_EXPRESSION_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "shellcore/ishell_core.h"

namespace mysh
{
  namespace mysqlx
  {
    class SHCORE_PUBLIC Expression : public shcore::Cpp_object_bridge
    {
    public:
      Expression(const std::string &expression) { _data = expression; }
      virtual ~Expression() {};

      // Virtual methods from object bridge
      virtual std::string class_name() const { return "Expression"; };
      virtual bool operator == (const Object_bridge &other) const;

      virtual shcore::Value get_member(const std::string &prop) const;
      std::vector<std::string> get_members() const;

      static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);

      std::string get_data() { return _data; };

    private:
      std::string _data;
    };
  };
};

#endif

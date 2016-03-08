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

// Interactive Expression access module
// Exposed as "Expression" in the shell

#ifndef _TANTS_H_
#define _MOD_MYSQLX_CONSTANTS_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

namespace mysh
{
    namespace mysqlx
    {
        class SHCORE_PUBLIC Type : public shcore::Cpp_object_bridge
        {
          public:
            // Virtual methods from object bridge
            virtual std::string class_name() const { return "mysqlx.Type"; };
            virtual bool operator == (const Object_bridge &other) const { return this == &other; };
      
            virtual shcore::Value get_member(const std::string &prop) const;
            std::vector<std::string> get_members() const;
      
            static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
          };
    
        class SHCORE_PUBLIC IndexType : public shcore::Cpp_object_bridge
        {
          public:
            // Virtual methods from object bridge
            virtual std::string class_name() const { return "mysqlx.IndexType"; };
            virtual bool operator == (const Object_bridge &other) const { return this == &other; }
      
            virtual shcore::Value get_member(const std::string &prop) const;
            std::vector<std::string> get_members() const;
      
            static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args);
          };
      };
  };

#endif
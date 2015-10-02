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

// Interactive session access module
// Exposed as "session" in the shell

#ifndef _MOD_CORE_RESULT_SET_H_
#define _MOD_CORE_RESULT_SET_H_

#include "mod_common.h"
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

namespace mysh
{
  class BaseResultset : public shcore::Cpp_object_bridge
  {
  public:
    BaseResultset();

    // Methods from Cpp_object_bridge will be defined here
    // Since all the connections will expose the same members
    virtual std::vector<std::string> get_members() const;
    virtual bool operator == (const Object_bridge &other) const;

    virtual shcore::Value next(const shcore::Argument_list &args) = 0;
    virtual shcore::Value all(const shcore::Argument_list &args) = 0;
    virtual shcore::Value next_result(const shcore::Argument_list &args) = 0;

    // Helper method to retrieve properties using a method
    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);
  };

  class SHCORE_PUBLIC Row : public shcore::Cpp_object_bridge
  {
  public:
    Row();
    virtual std::string class_name() const { return "Row"; }
    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);

  public:
    std::map<std::string, shcore::Value> values;
    std::vector<std::map<std::string, shcore::Value>::iterator> value_iterators;

    virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
    virtual std::string &append_repr(std::string &s_out) const;
    virtual void append_json(const shcore::JSON_dumper& dumper) const;

    //! Returns the list of members that this object has
    virtual std::vector<std::string> get_members() const;
    //! Implements equality operator
    virtual bool operator == (const Object_bridge &other) const;

    //! Returns the value of a member
    virtual shcore::Value get_member(const std::string &prop) const;
    shcore::Value get_member(size_t index) const;

    virtual bool is_indexed() const { return true; }

    void add_item(const std::string &key, shcore::Value value);
  };
};

#endif

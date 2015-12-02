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
  // This is the Shell Common Base Class for all the resultset classes
  class ShellBaseResult : public shcore::Cpp_object_bridge
  {
  public:
    virtual bool operator == (const Object_bridge &other) const;

    // Helper method to retrieve properties using a method
    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);

    // Doing nothing by default to avoid impacting the classic result
    virtual void buffer(){};
    virtual bool rewind(){ return false; }
    virtual bool tell(size_t &dataset, size_t &record){ return false; }
    virtual bool seek(size_t dataset, size_t record){ return false; }
  };

  class SHCORE_PUBLIC Column : public shcore::Cpp_object_bridge
  {
  public:
    Column(const std::string& catalog, const std::string& schema, const std::string& table, const std::string& org_table, const std::string& name,
           const std::string& org_name, uint64_t collation, uint64_t length, uint64_t type, uint64_t flags, uint64_t max_length, bool _numeric);

    virtual bool operator == (const Object_bridge &other) const;
    virtual std::string class_name() const { return "Column"; }
    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);

    virtual std::vector<std::string> get_members() const;
    virtual shcore::Value get_member(const std::string &prop) const;

    std::string get_catalog(){ return _catalog; }
    std::string get_schema(){ return _schema; }
    std::string get_table_name(){ return _table; }
    std::string get_original_table_name(){ return _org_table; }
    std::string get_name(){ return _name; }
    std::string get_original_name(){ return _org_name; }
    uint64_t get_collation(){ return _collation; }
    uint64_t get_length(){ return _length; }
    uint64_t get_type(){ return _type; }
    uint64_t get_flags(){ return _flags; }
    uint64_t get_max_length(){ return _max_length; }
    bool is_numeric(){ return _numeric; }

  private:
    std::string _catalog;
    std::string _schema;
    std::string _table;
    std::string _org_table;
    std::string _name;
    std::string _org_name;
    uint64_t _collation;
    uint64_t _length;
    uint64_t _type;
    uint64_t _flags;
    uint64_t _max_length;
    bool _numeric;
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
    virtual void append_json(shcore::JSON_dumper& dumper) const;

    shcore::Value get_field(const shcore::Argument_list &args);

    virtual bool has_member(const std::string &prop) const;
    //! Returns the list of members that this object has
    virtual std::vector<std::string> get_members() const;
    //! Implements equality operator
    virtual bool operator == (const Object_bridge &other) const;

    //! Returns the value of a member
    virtual shcore::Value get_member(const std::string &prop) const;
    shcore::Value get_member(size_t index) const;

    size_t get_length() { return values.size(); }
    virtual bool is_indexed() const { return true; }

    void add_item(const std::string &key, shcore::Value value);
  };
};

#endif

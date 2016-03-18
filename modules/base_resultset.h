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

  class SHCORE_PUBLIC Charset
  {
  private:
    typedef struct
    {
      uint32_t id;
      std::string name;
      std::string collation;
    } Charset_entry;

  public:
    static const Charset_entry item[];
  };

  /**
  * Represents the a Column definition on a result.
  */
  class SHCORE_PUBLIC Column : public shcore::Cpp_object_bridge
  {
  public:
    Column(const std::string& schema, const std::string& org_table, const std::string& table, const std::string& org_name, const std::string& name,
           shcore::Value type, uint64_t length, bool numeric, uint64_t fractional, bool is_signed, const std::string &collation, const std::string &charset, bool padded);

    virtual bool operator == (const Object_bridge &other) const;
    virtual std::string class_name() const { return "Column"; }
    shcore::Value get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop);

    virtual std::vector<std::string> get_members() const;
    virtual shcore::Value get_member(const std::string &prop) const;

    // Shell Specific for internal use
    bool is_numeric(){ return _numeric; }

    std::string get_schema_name(){ return _schema; }
    std::string get_table_name(){ return _table_name; }
    std::string get_table_label(){ return _table_label; }
    std::string get_column_name(){ return _column_name; }
    std::string get_column_label(){ return _column_label; }

    shcore::Value get_type(){ return _type; }
    uint64_t get_length(){ return _length; }

    uint64_t get_fractional_digits(){ return _fractional; }
    bool is_number_signed() { return _signed; }
    std::string get_collation_name() { return _collation; }
    std::string get_character_set_name() { return _charset; }
    bool is_padded() { return _padded; }

#ifdef DOXYGEN
    schemaName; //!< Same as getSchemaName()
    tableName; //!< Same as getTableName()
    tableLabel; //!< Same as getTableLabel()
    columnName; //!< Same as getColumnLabel()
    columnLabel; //!< Same as getLastInsertId()
    type; //!< Same as getType()
    length; //!< Same as getLength()
    fractionalDigits; //!< Same as getFractionalDigits()
    numberSigned; //!< Same as isNumberSigned()
    collationName; //!< Same as getCollationName()
    characterSetName; //!< Same as getCharacterSetName()
    padded; //!< Same as isPadded()

    String getSchemaName();
    String getTableName();
    String getTableLabel();
    String getColumnName();
    String getColumnLabel();
    Type getType();
    Integer getLength();
    Integer getFractionalDigits();
    Boolean isNumberSigned();
    String getCollationName();
    String getCharacterSetName();
    Boolean isPadded();
#endif

  private:
    std::string _schema;
    std::string _table_name;
    std::string _table_label;
    std::string _column_name;
    std::string _column_label;
    std::string _collation;
    std::string _charset;
    uint64_t _length;
    shcore::Value _type;
    uint64_t _fractional;
    bool _signed;
    bool _padded;
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

/*
 * Copyright (c) 2015, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "scripting/types.h"
#include "scripting/types_cpp.h"

namespace mysqlsh {
// This is the Shell Common Base Class for all the resultset classes
class ShellBaseResult : public shcore::Cpp_object_bridge {
public:
  virtual bool operator == (const Object_bridge &other) const;

  // Doing nothing by default to avoid impacting the classic result
  virtual void buffer() {};
  virtual bool rewind() { return false; }
  virtual bool tell(size_t &dataset, size_t &record) { return false; }
  virtual bool seek(size_t dataset, size_t record) { return false; }
};

class SHCORE_PUBLIC Charset {
private:
  typedef struct {
    uint32_t id;
    std::string name;
    std::string collation;
  } Charset_entry;

public:
  static const Charset_entry item[];
};

/**
* \ingroup ShellAPI
* Represents the a Column definition on a result.
*/
class SHCORE_PUBLIC Column : public shcore::Cpp_object_bridge {
public:
  Column(const std::string& schema, const std::string& org_table, const std::string& table, const std::string& org_name, const std::string& name,
         shcore::Value type, uint64_t length, bool numeric, uint64_t fractional, bool is_signed, const std::string &collation, const std::string &charset, bool padded);

  virtual bool operator == (const Object_bridge &other) const;
  virtual std::string class_name() const { return "Column"; }

  virtual shcore::Value get_member(const std::string &prop) const;

  // Shell Specific for internal use
  bool is_numeric() { return _numeric; }

#if DOXYGEN_JS
  schemaName; //!< Same as getSchemaName()
  tableName; //!< Same as getTableName()
  tableLabel; //!< Same as getTableLabel()
  columnName; //!< Same as getColumnName()
  columnLabel; //!< Same as getColumnLabel()
  type; //!< Same as getType()
  length; //!< Same as getLength()
  fractionalDigits; //!< Same as getFractionalDigits()
  numberSigned; //!< Same as isNumberSigned()
  collationName; //!< Same as getCollationName()
  characterSetName; //!< Same as getCharacterSetName()
  padded; //!< Same as isPadded()
#elif DOXYGEN_PY
  schema_name; //!< Same as get_schema_name()
  table_name; //!< Same as get_table_name()
  table_label; //!< Same as get_table_label()
  column_name; //!< Same as get_column_name()
  column_label; //!< Same as get_column_label()
  type; //!< Same as get_type()
  length; //!< Same as get_length()
  fractional_digits; //!< Same as get_fractional_digits()
  number_signed; //!< Same as is_number_signed()
  collation_name; //!< Same as get_collation_name()
  character_set_name; //!< Same as get_character_set_name()
  padded; //!< Same as is_padded()
#endif

  /**
   * Retrieves the name of the Schema where the column is defined.
   * \return a string value representing the owner schema.
   */
#if DOXYGEN_JS
  String getSchemaName() {};
#elif DOXYGEN_PY
  str get_schema_name() {};
#endif
  std::string get_schema_name() { return _schema; }

  /**
   * Retrieves table name where the column is defined.
   * \return a string value representing the table name.
   */
#if DOXYGEN_JS
  String getTableName() {};
#elif DOXYGEN_PY
  str get_table_name() {};
#endif
  std::string get_table_name() { return _table_name; }

  /**
   * Retrieves table alias where the column is defined.
   * \return a string value representing the table alias or the table name if no alias is defined.
   */
#if DOXYGEN_JS
  String getTableLabel() {};
#elif DOXYGEN_PY
  str get_table_label() {};
#endif
  std::string get_table_label() { return _table_label; }

  /**
   * Retrieves column name.
   * \return a string value representing the column name.
   */
#if DOXYGEN_JS
  String getColumnName() {};
#elif DOXYGEN_PY
  str get_column_name() {};
#endif
  std::string get_column_name() { return _column_name; }

  /**
   * Retrieves column alias.
   * \return a string value representing the column alias or the column name if no alias is defined.
   */
#if DOXYGEN_JS
  String getColumnLabel() {};
#elif DOXYGEN_PY
  str get_column_label() {};
#endif
  std::string get_column_label() { return _column_label; }

  /**
   * Retrieves column Type.
   * \return a constant value for the supported column types.
   */
#if DOXYGEN_JS
  Type getType() {};
#elif DOXYGEN_PY
  Type get_type() {};
#endif
  shcore::Value get_type() { return _type; }

  /**
   * Retrieves column length.
   * \return the column length.
   */
#if DOXYGEN_JS
  Integer getLength() {};
#elif DOXYGEN_PY
  int get_length() {};
#endif
  uint64_t get_length() { return _length; }

  /**
   * Retrieves the fractional digits if applicable
   * \return the number of fractional digits, this only applies to specific data types.
   */
#if DOXYGEN_JS
  Integer getFractionalDigits() {};
#elif DOXYGEN_PY
  int get_fractional_digits() {};
#endif
  uint64_t get_fractional_digits() { return _fractional; }

  /**
   * Indicates if a numeric column is signed.
   * \return a boolean indicating whether a numeric column is signed or not.
   */
#if DOXYGEN_JS
  Bool isNumberSigned() {};
#elif DOXYGEN_PY
  bool is_number_signed() {};
#endif
  bool is_number_signed() { return _signed; }

  /**
   * Retrieves the collation name
   * \return a String representing the collation name, aplicable only to specific data types.
   */
#if DOXYGEN_JS
  String getCollationName() {};
#elif DOXYGEN_PY
  str get_collation_name() {};
#endif
  std::string get_collation_name() { return _collation; }

  /**
   * Retrieves the character set name
   * \return a String representing the character set name, aplicable only to specific data types.
   */
#if DOXYGEN_JS
  String getCharacterSetName() {};
#elif DOXYGEN_PY
  str get_character_setName() {};
#endif
  std::string get_character_set_name() { return _charset; }

  /**
   * Indicates if padding is used for the column
   * \return a boolean indicating if padding is used on the column.
   */
#if DOXYGEN_JS
  Bool isPadded() {};
#elif DOXYGEN_PY
  bool is_padded() {};
#endif
  bool is_padded() { return _padded; }

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

/**
* \ingroup ShellAPI
* Represents the a Row in a Result.
*/
#if !DOXYGEN_CPP
/**
 * \b Dynamic \b Properties
 *
 * In addition to the length property documented above, when a row object is created,
 * its fields are exposed as properties of the Row object if two conditions are met:
 *
 * \li Its name must be a valid identifier: [_a-zA-Z][_a-zA-Z0-9]*
 * \li Its name must be different from the fixed members of this object: length, get_length and get_field
 *
 * In the case a field does not met these conditions, it must be retrieved through the
 */
#if DOXYGEN_JS
//! getField(String fieldName)
#else
//! get_field(str fieldName)
#endif
/**
 * function.
 */
#endif

class SHCORE_PUBLIC Row : public shcore::Cpp_object_bridge {
public:
#if DOXYGEN_JS
  length; //!< Same as getLength()

  Integer getLength();
  Value getField(String fieldName);
#elif DOXYGEN_PY
  length; //!< Same as get_length()

  int get_length();
  Value get_field(str fieldName);
#endif
  Row();
  virtual std::string class_name() const { return "Row"; }

  std::map<std::string, shcore::Value> values;
  std::vector<std::string> names;
  shcore::Value::Array_type value_array;

  virtual std::string &append_descr(std::string &s_out, int indent = -1, int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual void append_json(shcore::JSON_dumper& dumper) const;

  shcore::Value get_field(const shcore::Argument_list &args);
  shcore::Value get_field_(const std::string &field);

  virtual bool operator == (const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;
  shcore::Value get_member(size_t index) const;

  size_t get_length() { return values.size(); }
  virtual bool is_indexed() const { return true; }

  void add_item(const std::string &key, shcore::Value value);
};
};

#endif

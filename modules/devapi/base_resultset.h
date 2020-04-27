/*
 * Copyright (c) 2015, 2020, Oracle and/or its affiliates. All rights reserved.
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

// Interactive session access module
// Exposed as "session" in the shell

#ifndef MODULES_DEVAPI_BASE_RESULTSET_H_
#define MODULES_DEVAPI_BASE_RESULTSET_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "db/column.h"
#include "db/row.h"
#include "modules/mod_common.h"
#include "mysqlshdk/libs/db/result.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

namespace mysqlsh {
class Row;
// This is the Shell Common Base Class for all the resultset classes
class ShellBaseResult : public shcore::Cpp_object_bridge {
 public:
  ShellBaseResult();

  // Needed because expose() in the c-tor will end up in a call to class_name()
  std::string class_name() const override { return "ShellBaseResult"; }

  bool operator==(const Object_bridge &other) const override;
  virtual mysqlshdk::db::IResult *get_result() const = 0;
  virtual std::shared_ptr<std::vector<std::string>> get_column_names() const {
    return {};
  }
  std::vector<std::string> get_members() const override;

  std::unique_ptr<mysqlsh::Row> fetch_one_row() const;

  shcore::Dictionary_t fetch_one_object() const;

  void dump();
};

/**
 * \ingroup ShellAPI
 * Represents the a Column definition on a result.
 */
class SHCORE_PUBLIC Column : public shcore::Cpp_object_bridge {
 public:
  Column(const mysqlshdk::db::Column &meta, shcore::Value type);

  virtual bool operator==(const Object_bridge &other) const;
  virtual std::string class_name() const { return "Column"; }

  virtual shcore::Value get_member(const std::string &prop) const;

#if DOXYGEN_JS
  schemaName;        //!< Same as getSchemaName()
  tableName;         //!< Same as getTableName()
  tableLabel;        //!< Same as getTableLabel()
  columnName;        //!< Same as getColumnName()
  columnLabel;       //!< Same as getColumnLabel()
  type;              //!< Same as getType()
  length;            //!< Same as getLength()
  fractionalDigits;  //!< Same as getFractionalDigits()
  numberSigned;      //!< Same as isNumberSigned()
  collationName;     //!< Same as getCollationName()
  characterSetName;  //!< Same as getCharacterSetName()
  zeroFill;          //!< Same as isZeroFill()
#elif DOXYGEN_PY
  schema_name;         //!< Same as get_schema_name()
  table_name;          //!< Same as get_table_name()
  table_label;         //!< Same as get_table_label()
  column_name;         //!< Same as get_column_name()
  column_label;        //!< Same as get_column_label()
  type;                //!< Same as get_type()
  length;              //!< Same as get_length()
  fractional_digits;   //!< Same as get_fractional_digits()
  number_signed;       //!< Same as is_number_signed()
  collation_name;      //!< Same as get_collation_name()
  character_set_name;  //!< Same as get_character_set_name()
  zero_fill;           //!< Same as is_zero_fill()
#endif

/**
 * Retrieves the name of the Schema where the column is defined.
 * \return a string value representing the owner schema.
 */
#if DOXYGEN_JS
  String getSchemaName() {}
#elif DOXYGEN_PY
  str get_schema_name() {}
#endif
  const std::string &get_schema_name() const { return _c.get_schema(); }

/**
 * Retrieves table name where the column is defined.
 * \return a string value representing the table name.
 */
#if DOXYGEN_JS
  String getTableName() {}
#elif DOXYGEN_PY
  str get_table_name() {}
#endif
  const std::string &get_table_name() const { return _c.get_table_name(); }

/**
 * Retrieves table alias where the column is defined.
 * \return a string value representing the table alias or the table name if no
 * alias is defined.
 */
#if DOXYGEN_JS
  String getTableLabel() {}
#elif DOXYGEN_PY
  str get_table_label() {}
#endif
  const std::string &get_table_label() const { return _c.get_table_label(); }

/**
 * Retrieves column name.
 * \return a string value representing the column name.
 */
#if DOXYGEN_JS
  String getColumnName() {}
#elif DOXYGEN_PY
  str get_column_name() {}
#endif
  const std::string &get_column_name() const { return _c.get_column_name(); }

/**
 * Retrieves column alias.
 * \return a string value representing the column alias or the column name if no
 * alias is defined.
 */
#if DOXYGEN_JS
  String getColumnLabel() {}
#elif DOXYGEN_PY
  str get_column_label() {}
#endif
  const std::string &get_column_label() const { return _c.get_column_label(); }

/**
 * Retrieves column Type.
 * \return a constant value for the supported column types.
 */
#if DOXYGEN_JS
  Type getType() {}
#elif DOXYGEN_PY
  Type get_type() {}
#endif
  shcore::Value get_type() const { return _type; }

/**
 * Retrieves column length.
 * \return the column length.
 */
#if DOXYGEN_JS
  Integer getLength() {}
#elif DOXYGEN_PY
  int get_length() {}
#endif
  uint32_t get_length() const { return _c.get_length(); }

/**
 * Retrieves the fractional digits if applicable
 * \return the number of fractional digits, this only applies to specific data
 * types.
 */
#if DOXYGEN_JS
  Integer getFractionalDigits() {}
#elif DOXYGEN_PY
  int get_fractional_digits() {}
#endif
  int get_fractional_digits() const { return _c.get_fractional(); }

/**
 * Indicates if a numeric column is signed.
 * \return a boolean indicating whether a numeric column is signed or not.
 */
#if DOXYGEN_JS
  Bool isNumberSigned() {}
#elif DOXYGEN_PY
  bool is_number_signed() {}
#endif
  bool is_number_signed() const {
    return _c.is_numeric() ? !_c.is_unsigned() : false;
  }

/**
 * Retrieves the collation name
 * \return a String representing the collation name, aplicable only to specific
 * data types.
 */
#if DOXYGEN_JS
  String getCollationName() {}
#elif DOXYGEN_PY
  str get_collation_name() {}
#endif
  std::string get_collation_name() const { return _c.get_collation_name(); }

/**
 * Retrieves the character set name
 * \return a String representing the character set name, aplicable only to
 * specific data types.
 */
#if DOXYGEN_JS
  String getCharacterSetName() {}
#elif DOXYGEN_PY
  str get_character_setName() {}
#endif
  std::string get_character_set_name() const { return _c.get_charset_name(); }

/**
 * Indicates if zerofill is set for the column
 * \return a boolean indicating if zerofill flag is set for the column.
 */
#if DOXYGEN_JS
  Bool isZeroFill() {}
#elif DOXYGEN_PY
  bool is_zero_fill() {}
#endif
  bool is_zerofill() const { return _c.is_zerofill(); }

  bool is_binary() const { return _c.is_binary(); }
  bool is_numeric() const { return _c.is_numeric(); }

 private:
  mysqlshdk::db::Column _c;
  shcore::Value _type;
};

/**
 * \ingroup ShellAPI
 *
 * Represents the a Row in a Result.
 *
 * $(ROW_DETAIL)
 * $(ROW_DETAIL1)
 * $(ROW_DETAIL2)
 *
 * In the case a field does not met these conditions, it must be retrieved
 * through the&nbsp;
 */
#if DOXYGEN_JS
//! getField(String name)
#else
//! get_field(str name)
#endif
/**
 * function.
 */

class SHCORE_PUBLIC Row : public shcore::Cpp_object_bridge {
 public:
#if DOXYGEN_JS
  length;  //!< Same as getLength()

  Integer getLength();
  Value getField(String fieldName);
#elif DOXYGEN_PY
  length;  //!< Same as get_length()

  int get_length();
  Value get_field(str fieldName);
#endif

  Row();
  Row(std::shared_ptr<std::vector<std::string>> names,
      const mysqlshdk::db::IRow &row);

  virtual std::string class_name() const { return "Row"; }

  std::shared_ptr<std::vector<std::string>> names;
  std::vector<shcore::Value> value_array;

  virtual std::string &append_descr(std::string &s_out, int indent = -1,
                                    int quote_strings = 0) const;
  virtual std::string &append_repr(std::string &s_out) const;
  virtual void append_json(shcore::JSON_dumper &dumper) const;

  shcore::Value get_field(const shcore::Argument_list &args);
  shcore::Value get_field_(const std::string &field) const;

  virtual bool operator==(const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;
  shcore::Value get_member(size_t index) const;

  size_t get_length() { return value_array.size(); }
  virtual bool is_indexed() const { return true; }

  void add_item(const std::string &key, shcore::Value value);

  shcore::Dictionary_t as_object();
};
}  // namespace mysqlsh

#endif  // MODULES_DEVAPI_BASE_RESULTSET_H_

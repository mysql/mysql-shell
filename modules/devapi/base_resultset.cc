/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/base_resultset.h"
#include <map>
#include <string>
#include <algorithm>

#include "scripting/common.h"
#include "scripting/lang_base.h"
#include "scripting/object_factory.h"
#include "shellcore/shell_core.h"
#include "utils/utils_general.h"

using namespace mysqlsh;
using namespace shcore;

bool ShellBaseResult::operator==(const Object_bridge &other) const {
  return this == &other;
}

Column::Column(const std::string &schema, const std::string &table_name,
               const std::string &table_label, const std::string &column_name,
               const std::string &column_label, shcore::Value type,
               uint64_t length, bool numeric, uint64_t fractional,
               bool is_signed, const std::string &collation,
               const std::string &charset, bool padded, bool zerofill)
    : _schema(schema),
      _table_name(table_name),
      _table_label(table_label),
      _column_name(column_name),
      _column_label(column_label),
      _collation(collation),
      _charset(charset),
      _length(length),
      _type(type),
      _fractional(fractional),
      _signed(is_signed),
      _padded(padded),
      _zerofill(zerofill),
      _numeric(numeric) {
  add_property("schemaName", "getSchemaName");
  add_property("tableName", "getTableName");
  add_property("tableLabel", "getTableLabel");
  add_property("columnName", "getColumnName");
  add_property("columnLabel", "getColumnLabel");
  add_property("type", "getType");
  add_property("length", "getLength");
  add_property("fractionalDigits", "getFractionalDigits");
  add_property("numberSigned", "isNumberSigned");
  add_property("collationName", "getCollationName");
  add_property("characterSetName", "getCharacterSetName");
  add_property("padded", "isPadded");
  add_property("zeroFill", "isZeroFill");
}

bool Column::operator==(const Object_bridge &other) const {
  return this == &other;
}
#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the
 * scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this
 * function. The content of the returned value depends on the property being
 * requested. The next list shows the valid properties as well as the returned
 * value for each of them:
 *
 * \li schemaName: returns a String object with the name of the Schema to which
 * this Column belongs.
 * \li tableName: returns a String object with the name of the Table to which
 * this Column belongs.
 * \li tableLabel: returns a String object with the alias of the Table to which
 * this Column belongs.
 * \li columnName: returns a String object with the name of the this Column.
 * \li columnLabel: returns a String object with the alias of this Column.
 * \li type: returns a Type object with the information about this Column data
 * type.
 * \li length: returns an uint64_t value with the length in bytes of this
 * Column.
 * \li fractionalDigits: returns an uint64_t value with the number of fractional
 * digits on this Column (Only applies to certain data types).
 * \li numberSigned: returns an boolean value indicating wether a numeric Column
 * is signed.
 * \li collationName: returns a String object with the collation name of the
 * Column.
 * \li characterSetName: returns a String object with the collation name of the
 * Column.
 * \li padded: returns an boolean value indicating wether the Column is padded.
 * \li zeroFill: returns an boolean value indicating wether the Column is to be
 * left padded with 0s.
 */
#endif
shcore::Value Column::get_member(const std::string &prop) const {
  shcore::Value ret_val;
  if (prop == "schemaName")
    ret_val = shcore::Value(_schema);
  else if (prop == "tableName")
    ret_val = shcore::Value(_table_name);
  else if (prop == "tableLabel")
    ret_val = shcore::Value(_table_label);
  else if (prop == "columnName")
    ret_val = shcore::Value(_column_name);
  else if (prop == "columnLabel")
    ret_val = shcore::Value(_column_label);
  else if (prop == "type")
    ret_val = shcore::Value(_type);
  else if (prop == "length")
    ret_val = shcore::Value(_length);
  else if (prop == "fractionalDigits")
    ret_val = shcore::Value(_fractional);
  else if (prop == "numberSigned")
    ret_val = shcore::Value(_signed);
  else if (prop == "collationName")
    ret_val = shcore::Value(_collation);
  else if (prop == "characterSetName")
    ret_val = shcore::Value(_charset);
  else if (prop == "padded")
    ret_val = shcore::Value(_padded);
  else if (prop == "zeroFill")
    ret_val = shcore::Value(_zerofill);
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);

  return ret_val;
}

Row::Row() {
  add_property("length", "getLength");
  add_method("getField", std::bind(&Row::get_field, this, _1), "field",
             shcore::String, NULL);
}

std::string &Row::append_descr(std::string &s_out, int indent,
                               int UNUSED(quote_strings)) const {
  std::string nl = (indent >= 0) ? "\n" : "";
  s_out += "[";
  for (size_t index = 0; index < value_array.size(); index++) {
    if (index > 0)
      s_out += ",";

    s_out += nl;

    if (indent >= 0)
      s_out.append((indent + 1) * 4, ' ');

    value_array[index].append_descr(s_out, indent < 0 ? indent :
      indent + 1, '"');
  }

  s_out += nl;
  if (indent > 0)
    s_out.append(indent * 4, ' ');

  s_out += "]";

  return s_out;
}

void Row::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();

  for (size_t index = 0; index < value_array.size(); index++)
    dumper.append_value(names[index], value_array[index]);

  dumper.end_object();
}

std::string &Row::append_repr(std::string &s_out) const {
  return append_descr(s_out);
}

bool Row::operator==(const Object_bridge &UNUSED(other)) const {
  return false;
}

//! Returns the value of a field on the Row based on the field name.
#if DOXYGEN_CPP
//! \param args : Should contain the name of the field to be returned
#else
//! \param fieldName : The name of the field to be returned
#endif
#if DOXYGEN_JS
Object Row::getField(String fieldName) {}
#elif DOXYGEN_PY
object Row::get_field(str fieldName) {}
#endif
shcore::Value Row::get_field(const shcore::Argument_list &args) {
  shcore::Value ret_val;
  args.ensure_count(1, "Row.getField");

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(
        "Row.getField: Argument #1 is expected to be a string");

  return get_field_(args[0].as_string());
}

shcore::Value Row::get_field_(const std::string &field) {
  auto iter = std::find(names.begin(), names.end(), field);
  if (iter != names.end())
    return value_array[iter - names.begin()];
  else
    throw shcore::Exception::argument_error("Row.getField: Field " + field +
                                            " does not exist");
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the
 * scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this
 * function. The the content of the returned value depends on the property being
 * requested. The next list shows the valid properties as well as the returned
 * value for each of them:
 *
 * \li length: returns the number of fields contained on this Row object.
 * \li Each field is exposed as a member of this Row object, if prop is a valid
 * field name the value for that field will be returned.
 *
 * NOTE: if a field of on the Row is named "length", it´s value must be
 * retrieved using the get_field() function.
 */
#else
/**
 *  Returns the number of field contained on this Row object.
 */
#if DOXYGEN_JS
Integer Row::getLength() {}
#elif DOXYGEN_PY
int Row::get_length() {}
#endif
#endif
shcore::Value Row::get_member(const std::string &prop) const {
  if (prop == "length")
    return shcore::Value((int)value_array.size());
  else {
    auto it = std::find(names.begin(), names.end(), prop);
    if (it != names.end())
      return value_array[it - names.begin()];
  }

  return shcore::Cpp_object_bridge::get_member(prop);
}

#if DOXYGEN_CPP
/**
 * Returns the value of a field on the Row based on the field position.
 */
#endif
shcore::Value Row::get_member(size_t index) const {
  if (index < value_array.size())
    return value_array[index];
  else
    return shcore::Value();
}

void Row::add_item(const std::string &key, shcore::Value value) {

  // All the values are available through index
  value_array.push_back(value);
  names.push_back(key);

  // Values would be available as properties if they are valid identifier
  // and not base members like lenght and getField
  // O on this case the values would be available as
  // row.property
  if (shcore::is_valid_identifier(key) && !has_member(key))
    add_property(key);
}

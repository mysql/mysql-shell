/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "modules/devapi/base_resultset.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>

#include "modules/devapi/base_constants.h"
#include "modules/mod_utils.h"
#include "mysqlshdk/include/scripting/common.h"
#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/obj_date.h"
#include "mysqlshdk/include/scripting/object_factory.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/shell_core.h"
#include "mysqlshdk/include/shellcore/shell_resultset_dumper.h"  // TODO(alfredo) - move this to modules/
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"

using namespace mysqlsh;
using namespace shcore;

REGISTER_HELP_CLASS(Column, shellapi);
REGISTER_HELP(COLUMN_BRIEF,
              "Represents the metadata for a column in a result.");

ShellBaseResult::ShellBaseResult() { expose("dump", &ShellBaseResult::dump); }

bool ShellBaseResult::operator==(const Object_bridge &other) const {
  return this == &other;
}

std::vector<std::string> ShellBaseResult::get_members() const {
  std::vector<std::string> members = Cpp_object_bridge::get_members();

  // The dump function is used as a shell_hook but is not officially part of
  // the Result classes in DevAPI, for this reason it is exposed to make it
  // usable as a hook, but removed in get_members so it is not part of the
  // Result classes
  auto pos = std::find(members.begin(), members.end(), "dump");

  assert(pos != members.end());
  if (pos != members.end()) members.erase(pos);

  return members;
}

void ShellBaseResult::dump() {
  auto result = get_result();
  if (result == nullptr) return;
  result->buffer();

  bool is_result = class_name() == "Result";
  bool is_doc_result = class_name() == "DocResult";
  bool is_row_result = class_name() == "RowResult";

  bool is_query = is_doc_result || is_row_result;
  std::string item_label =
      is_doc_result ? "document" : is_result ? "item" : "row";

  mysqlsh::dump_result(result, item_label, is_query, is_doc_result);

  result->rewind();
}

std::unique_ptr<mysqlsh::Row> ShellBaseResult::fetch_one_row() const {
  std::unique_ptr<mysqlsh::Row> ret_val;

  auto result = get_result();
  auto columns = get_column_names();
  if (result && columns) {
    const mysqlshdk::db::IRow *row = result->fetch_one();
    if (row) {
      ret_val = std::make_unique<mysqlsh::Row>(columns, *row);
    }
  }

  return ret_val;
}

shcore::Dictionary_t ShellBaseResult::fetch_one_object() const {
  if (auto row = fetch_one_row(); row) return row->as_object();
  return {};
}

std::shared_ptr<std::vector<std::string>> ShellBaseResult::get_column_names()
    const {
  update_column_cache();

  return m_column_names;
}

void ShellBaseResult::reset_column_cache() const {
  m_columns.reset();
  m_column_names.reset();
}

void ShellBaseResult::update_column_cache() const {
  if (has_data() && !m_columns && !m_column_names) {
    m_columns = std::make_shared<shcore::Value::Array_type>();
    m_column_names = std::make_shared<std::vector<std::string>>();

    for (auto &column_meta : get_metadata()) {
      std::string type_name = mysqlshdk::db::type_to_dbstring(
          column_meta.get_type(), column_meta.get_length());

      shcore::Value data_type = mysqlsh::Constant::get_constant(
          get_protocol(), "Type", shcore::str_upper(type_name),
          shcore::Argument_list());

      m_columns->push_back(
          shcore::Value::wrap(new mysqlsh::Column(column_meta, data_type)));

      m_column_names->push_back(column_meta.get_column_label());
    }
  }
}

Column::Column(const mysqlshdk::db::Column &meta, shcore::Value type)
    : _c(meta), _type(type) {
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
  add_property("zeroFill", "isZeroFill");
  add_property("flags", "getFlags");
}

bool Column::operator==(const Object_bridge &other) const {
  return this == &other;
}

REGISTER_HELP_PROPERTY(schemaName, Column);
REGISTER_HELP_PROPERTY(tableName, Column);
REGISTER_HELP_PROPERTY(tableLabel, Column);
REGISTER_HELP_PROPERTY(columnName, Column);
REGISTER_HELP_PROPERTY(columnLabel, Column);
REGISTER_HELP_PROPERTY(type, Column);
REGISTER_HELP_PROPERTY(length, Column);
REGISTER_HELP_PROPERTY(fractionalDigits, Column);
REGISTER_HELP_PROPERTY(numberSigned, Column);
REGISTER_HELP_PROPERTY(collationName, Column);
REGISTER_HELP_PROPERTY(characterSetName, Column);
REGISTER_HELP_PROPERTY(zeroFill, Column);
REGISTER_HELP_PROPERTY(flags, Column);

REGISTER_HELP_FUNCTION(getSchemaName, Column);
REGISTER_HELP_FUNCTION(getTableName, Column);
REGISTER_HELP_FUNCTION(getTableLabel, Column);
REGISTER_HELP_FUNCTION(getColumnName, Column);
REGISTER_HELP_FUNCTION(getColumnLabel, Column);
REGISTER_HELP_FUNCTION(getType, Column);
REGISTER_HELP_FUNCTION(getLength, Column);
REGISTER_HELP_FUNCTION(getFractionalDigits, Column);
REGISTER_HELP_FUNCTION(getNumberSigned, Column);
REGISTER_HELP_FUNCTION(getCollationName, Column);
REGISTER_HELP_FUNCTION(getCharacterSetName, Column);
REGISTER_HELP_FUNCTION(getZeroFill, Column);
REGISTER_HELP_FUNCTION(getFlags, Column);
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
 * \li flags: returns a space separated list of database flags set for the
 * Column.
 */
#endif
shcore::Value Column::get_member(const std::string &prop) const {
  if (prop == "schemaName") return shcore::Value(_c.get_schema());
  if (prop == "tableName") return shcore::Value(_c.get_table_name());
  if (prop == "tableLabel") return shcore::Value(_c.get_table_label());
  if (prop == "columnName") return shcore::Value(_c.get_column_name());
  if (prop == "columnLabel") return shcore::Value(_c.get_column_label());
  if (prop == "type") return shcore::Value(_type);
  if (prop == "length") return shcore::Value(_c.get_length());
  if (prop == "fractionalDigits") return shcore::Value(_c.get_fractional());
  if (prop == "numberSigned") return shcore::Value(is_number_signed());
  if (prop == "collationName") return shcore::Value(_c.get_collation_name());
  if (prop == "characterSetName") return shcore::Value(_c.get_charset_name());
  if (prop == "zeroFill") return shcore::Value(_c.is_zerofill());
  if (prop == "flags") return shcore::Value(_c.get_flags());
  return shcore::Cpp_object_bridge::get_member(prop);
}

REGISTER_HELP_CLASS(Row, shellapi);
REGISTER_HELP_CLASS_TEXT(ROW, R"*(
Represents the a Row in a Result.

When a row object is created, its fields are exposed as properties of the Row
object if two conditions are met:

@li Its name must be a valid identifier: [_a-zA-Z][_a-zA-Z0-9]*
@li Its name must be different from names of the members of this object.


In the case a field does not met these conditions, it must be retrieved through
the Row.<<<getField>>>(@<field_name@>) function.
)*");
Row::Row() {
  add_property("length", "getLength");
  expose("getField", &Row::get_field, "fieldName");
  names.reset(new std::vector<std::string>());
}

Row::Row(std::shared_ptr<std::vector<std::string>> names_,
         const mysqlshdk::db::IRow &row)
    : names(names_) {
  assert(names);

  add_property("length", "getLength");
  expose("getField", &Row::get_field, "fieldName");

  assert(row.num_fields() == names_->size());
  for (uint32_t i = 0, c = row.num_fields(); i < c; i++) {
    const std::string &key = names_->at(i);
    // Values would be available as properties if they are valid identifier
    // and not base members like length and getField
    // O on this case the values would be available as
    // row.property
    // Properties for Row Fields are exposed exactly as the field name in both
    // JavaScript and Python, hence the property is registered as
    // <jsname> | <pyname> to avoid the naming enforcement logic to mess with
    // the property name.
    // i.e. without this a property like NAME would be turned into n_a_m_e for
    // Python
    if (shcore::is_valid_identifier(key) && !has_member(key))
      add_property(key + "|" + key);
  }

  value_array = get_row_values(row);
}

shcore::Dictionary_t Row::as_object() {
  auto ret_val = shcore::make_dict();

  for (size_t index = 0; index < names->size(); index++) {
    ret_val->emplace(names->at(index), value_array.at(index));
  }

  return ret_val;
}

std::string &Row::append_descr(std::string &s_out, int indent,
                               int UNUSED(quote_strings)) const {
  std::string nl = (indent >= 0) ? "\n" : "";
  s_out += "[";
  for (size_t index = 0; index < value_array.size(); index++) {
    if (index > 0) s_out += ", ";

    s_out += nl;

    if (indent >= 0) s_out.append((indent + 1) * 4, ' ');

    value_array[index].append_descr(s_out, indent < 0 ? indent : indent + 1,
                                    '"');
  }

  s_out += nl;
  if (indent > 0) s_out.append(indent * 4, ' ');

  s_out += "]";

  return s_out;
}

void Row::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();

  for (size_t index = 0; index < value_array.size(); index++)
    dumper.append_value(names->at(index), value_array[index]);

  dumper.end_object();
}

std::string &Row::append_repr(std::string &s_out) const {
  return append_descr(s_out);
}

bool Row::operator==(const Object_bridge &UNUSED(other)) const { return false; }

REGISTER_HELP_FUNCTION(getField, Row);
REGISTER_HELP_FUNCTION_TEXT(ROW_GETFIELD, R"*(
Returns the value of the field named <b>name</b>.

@param name The name of the field to be retrieved.
)*");
/**
 * $(ROW_GETFIELD_BRIEF)
 *
 * $(ROW_GETFIELD)
 */
#if DOXYGEN_JS
Object Row::getField(String name) {}
#elif DOXYGEN_PY
object Row::get_field(str name) {}
#endif
shcore::Value Row::get_field(const std::string &name) const {
  auto iter = std::find(names->begin(), names->end(), name);
  if (iter != names->end())
    return value_array[iter - names->begin()];
  else
    throw shcore::Exception::argument_error("Field " + name +
                                            " does not exist");
}

REGISTER_HELP_FUNCTION(getLength, Row);
REGISTER_HELP(ROW_GETLENGTH_BRIEF, "Returns number of fields in the Row.");
REGISTER_HELP_PROPERTY(length, Row);
REGISTER_HELP(ROW_LENGTH_BRIEF, "${ROW_GETLENGTH_BRIEF}");
/**
 * $(ROW_GETLENGTH_BRIEF)
 */
#if DOXYGEN_JS
Integer Row::getLength() {}
#elif DOXYGEN_PY
int Row::get_length() {}
#endif
shcore::Value Row::get_member(const std::string &prop) const {
  if (prop == "length") {
    return shcore::Value((int)value_array.size());
  } else {
    auto it = std::find(names->begin(), names->end(), prop);
    if (it != names->end()) return value_array[it - names->begin()];
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
  names->push_back(key);

  // Values would be available as properties if they are valid identifier
  // and not base members like lenght and getField
  // O on this case the values would be available as
  // row.property
  if (shcore::is_valid_identifier(key) && !has_member(key)) add_property(key);
}

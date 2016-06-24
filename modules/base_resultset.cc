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

#include "base_resultset.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/common.h"
#include "utils/utils_general.h"

#include <boost/bind.hpp>
#include <boost/pointer_cast.hpp>

using namespace mysh;
using namespace shcore;

const Charset::Charset_entry Charset::item[] = {
  { 0, "", "" },
  { 1, "big5", "big5_chinese_ci" },
  { 2, "latin2", "latin2_czech_cs" },
  { 3, "dec8", "dec8_swedish_ci" },
  { 4, "cp850", "cp850_general_ci" },
  { 5, "latin1", "latin1_german1_ci" },
  { 6, "hp8", "hp8_english_ci" },
  { 7, "koi8r", "koi8r_general_ci" },
  { 8, "latin1", "latin1_swedish_ci" },
  { 9, "latin2", "latin2_general_ci" },
  { 10, "swe7", "swe7_swedish_ci" },
  { 11, "ascii", "ascii_general_ci" },
  { 12, "ujis", "ujis_japanese_ci" },
  { 13, "sjis", "sjis_japanese_ci" },
  { 14, "cp1251", "cp1251_bulgarian_ci" },
  { 15, "latin1", "latin1_danish_ci" },
  { 16, "hebrew", "hebrew_general_ci" },
  { 18, "tis620", "tis620_thai_ci" },
  { 19, "euckr", "euckr_korean_ci" },
  { 20, "latin7", "latin7_estonian_cs" },
  { 21, "latin2", "latin2_hungarian_ci" },
  { 22, "koi8u", "koi8u_general_ci" },
  { 23, "cp1251", "cp1251_ukrainian_ci" },
  { 24, "gb2312", "gb2312_chinese_ci" },
  { 25, "greek", "greek_general_ci" },
  { 26, "cp1250", "cp1250_general_ci" },
  { 27, "latin2", "latin2_croatian_ci" },
  { 28, "gbk", "gbk_chinese_ci" },
  { 29, "cp1257", "cp1257_lithuanian_ci" },
  { 30, "latin5", "latin5_turkish_ci" },
  { 31, "latin1", "latin1_german2_ci" },
  { 32, "armscii8", "armscii8_general_ci" },
  { 33, "utf8", "utf8_general_ci" },
  { 34, "cp1250", "cp1250_czech_cs" },
  { 35, "ucs2", "ucs2_general_ci" },
  { 36, "cp866", "cp866_general_ci" },
  { 37, "keybcs2", "keybcs2_general_ci" },
  { 38, "macce", "macce_general_ci" },
  { 39, "macroman", "macroman_general_ci" },
  { 40, "cp852", "cp852_general_ci" },
  { 41, "latin7", "latin7_general_ci" },
  { 42, "latin7", "latin7_general_cs" },
  { 43, "macce", "macce_bin" },
  { 44, "cp1250", "cp1250_croatian_ci" },
  { 45, "utf8mb4", "utf8mb4_general_ci" },
  { 46, "utf8mb4", "utf8mb4_bin" },
  { 47, "latin1", "latin1_bin" },
  { 48, "latin1", "latin1_general_ci" },
  { 49, "latin1", "latin1_general_cs" },
  { 50, "cp1251", "cp1251_bin" },
  { 51, "cp1251", "cp1251_general_ci" },
  { 52, "cp1251", "cp1251_general_cs" },
  { 53, "macroman", "macroman_bin" },
  { 54, "utf16", "utf16_general_ci" },
  { 55, "utf16", "utf16_bin" },
  { 56, "utf16le", "utf16le_general_ci" },
  { 57, "cp1256", "cp1256_general_ci" },
  { 58, "cp1257", "cp1257_bin" },
  { 59, "cp1257", "cp1257_general_ci" },
  { 60, "utf32", "utf32_general_ci" },
  { 61, "utf32", "utf32_bin" },
  { 62, "utf16le", "utf16le_bin" },
  { 63, "binary", "binary" },
  { 64, "armscii8", "armscii8_bin" },
  { 65, "ascii", "ascii_bin" },
  { 66, "cp1250", "cp1250_bin" },
  { 67, "cp1256", "cp1256_bin" },
  { 68, "cp866", "cp866_bin" },
  { 69, "dec8", "dec8_bin" },
  { 70, "greek", "greek_bin" },
  { 71, "hebrew", "hebrew_bin" },
  { 72, "hp8", "hp8_bin" },
  { 73, "keybcs2", "keybcs2_bin" },
  { 74, "koi8r", "koi8r_bin" },
  { 75, "koi8u", "koi8u_bin" },
  { 77, "latin2", "latin2_bin" },
  { 78, "latin5", "latin5_bin" },
  { 79, "latin7", "latin7_bin" },
  { 80, "cp850", "cp850_bin" },
  { 81, "cp852", "cp852_bin" },
  { 82, "swe7", "swe7_bin" },
  { 83, "utf8", "utf8_bin" },
  { 84, "big5", "big5_bin" },
  { 85, "euckr", "euckr_bin" },
  { 86, "gb2312", "gb2312_bin" },
  { 87, "gbk", "gbk_bin" },
  { 88, "sjis", "sjis_bin" },
  { 89, "tis620", "tis620_bin" },
  { 90, "ucs2", "ucs2_bin" },
  { 91, "ujis", "ujis_bin" },
  { 92, "geostd8", "geostd8_general_ci" },
  { 93, "geostd8", "geostd8_bin" },
  { 94, "latin1", "latin1_spanish_ci" },
  { 95, "cp932", "cp932_japanese_ci" },
  { 96, "cp932", "cp932_bin" },
  { 97, "eucjpms", "eucjpms_japanese_ci" },
  { 98, "eucjpms", "eucjpms_bin" },
  { 99, "cp1250", "cp1250_polish_ci" },
  { 101, "utf16", "utf16_unicode_ci" },
  { 102, "utf16", "utf16_icelandic_ci" },
  { 103, "utf16", "utf16_latvian_ci" },
  { 104, "utf16", "utf16_romanian_ci" },
  { 105, "utf16", "utf16_slovenian_ci" },
  { 106, "utf16", "utf16_polish_ci" },
  { 107, "utf16", "utf16_estonian_ci" },
  { 108, "utf16", "utf16_spanish_ci" },
  { 109, "utf16", "utf16_swedish_ci" },
  { 110, "utf16", "utf16_turkish_ci" },
  { 111, "utf16", "utf16_czech_ci" },
  { 112, "utf16", "utf16_danish_ci" },
  { 113, "utf16", "utf16_lithuanian_ci" },
  { 114, "utf16", "utf16_slovak_ci" },
  { 115, "utf16", "utf16_spanish2_ci" },
  { 116, "utf16", "utf16_roman_ci" },
  { 117, "utf16", "utf16_persian_ci" },
  { 118, "utf16", "utf16_esperanto_ci" },
  { 119, "utf16", "utf16_hungarian_ci" },
  { 120, "utf16", "utf16_sinhala_ci" },
  { 121, "utf16", "utf16_german2_ci" },
  { 122, "utf16", "utf16_croatian_ci" },
  { 123, "utf16", "utf16_unicode_520_ci" },
  { 124, "utf16", "utf16_vietnamese_ci" },
  { 128, "ucs2", "ucs2_unicode_ci" },
  { 129, "ucs2", "ucs2_icelandic_ci" },
  { 130, "ucs2", "ucs2_latvian_ci" },
  { 131, "ucs2", "ucs2_romanian_ci" },
  { 132, "ucs2", "ucs2_slovenian_ci" },
  { 133, "ucs2", "ucs2_polish_ci" },
  { 134, "ucs2", "ucs2_estonian_ci" },
  { 135, "ucs2", "ucs2_spanish_ci" },
  { 136, "ucs2", "ucs2_swedish_ci" },
  { 137, "ucs2", "ucs2_turkish_ci" },
  { 138, "ucs2", "ucs2_czech_ci" },
  { 139, "ucs2", "ucs2_danish_ci" },
  { 140, "ucs2", "ucs2_lithuanian_ci" },
  { 141, "ucs2", "ucs2_slovak_ci" },
  { 142, "ucs2", "ucs2_spanish2_ci" },
  { 143, "ucs2", "ucs2_roman_ci" },
  { 144, "ucs2", "ucs2_persian_ci" },
  { 145, "ucs2", "ucs2_esperanto_ci" },
  { 146, "ucs2", "ucs2_hungarian_ci" },
  { 147, "ucs2", "ucs2_sinhala_ci" },
  { 148, "ucs2", "ucs2_german2_ci" },
  { 149, "ucs2", "ucs2_croatian_ci" },
  { 150, "ucs2", "ucs2_unicode_520_ci" },
  { 151, "ucs2", "ucs2_vietnamese_ci" },
  { 159, "ucs2", "ucs2_general_mysql500_ci" },
  { 160, "utf32", "utf32_unicode_ci" },
  { 161, "utf32", "utf32_icelandic_ci" },
  { 162, "utf32", "utf32_latvian_ci" },
  { 163, "utf32", "utf32_romanian_ci" },
  { 164, "utf32", "utf32_slovenian_ci" },
  { 165, "utf32", "utf32_polish_ci" },
  { 166, "utf32", "utf32_estonian_ci" },
  { 167, "utf32", "utf32_spanish_ci" },
  { 168, "utf32", "utf32_swedish_ci" },
  { 169, "utf32", "utf32_turkish_ci" },
  { 170, "utf32", "utf32_czech_ci" },
  { 171, "utf32", "utf32_danish_ci" },
  { 172, "utf32", "utf32_lithuanian_ci" },
  { 173, "utf32", "utf32_slovak_ci" },
  { 174, "utf32", "utf32_spanish2_ci" },
  { 175, "utf32", "utf32_roman_ci" },
  { 176, "utf32", "utf32_persian_ci" },
  { 177, "utf32", "utf32_esperanto_ci" },
  { 178, "utf32", "utf32_hungarian_ci" },
  { 179, "utf32", "utf32_sinhala_ci" },
  { 180, "utf32", "utf32_german2_ci" },
  { 181, "utf32", "utf32_croatian_ci" },
  { 182, "utf32", "utf32_unicode_520_ci" },
  { 183, "utf32", "utf32_vietnamese_ci" },
  { 192, "utf8", "utf8_unicode_ci" },
  { 193, "utf8", "utf8_icelandic_ci" },
  { 194, "utf8", "utf8_latvian_ci" },
  { 195, "utf8", "utf8_romanian_ci" },
  { 196, "utf8", "utf8_slovenian_ci" },
  { 197, "utf8", "utf8_polish_ci" },
  { 198, "utf8", "utf8_estonian_ci" },
  { 199, "utf8", "utf8_spanish_ci" },
  { 200, "utf8", "utf8_swedish_ci" },
  { 201, "utf8", "utf8_turkish_ci" },
  { 202, "utf8", "utf8_czech_ci" },
  { 203, "utf8", "utf8_danish_ci" },
  { 204, "utf8", "utf8_lithuanian_ci" },
  { 205, "utf8", "utf8_slovak_ci" },
  { 206, "utf8", "utf8_spanish2_ci" },
  { 207, "utf8", "utf8_roman_ci" },
  { 208, "utf8", "utf8_persian_ci" },
  { 209, "utf8", "utf8_esperanto_ci" },
  { 210, "utf8", "utf8_hungarian_ci" },
  { 211, "utf8", "utf8_sinhala_ci" },
  { 212, "utf8", "utf8_german2_ci" },
  { 213, "utf8", "utf8_croatian_ci" },
  { 214, "utf8", "utf8_unicode_520_ci" },
  { 215, "utf8", "utf8_vietnamese_ci" },
  { 223, "utf8", "utf8_general_mysql500_ci" },
  { 224, "utf8mb4", "utf8mb4_unicode_ci" },
  { 225, "utf8mb4", "utf8mb4_icelandic_ci" },
  { 226, "utf8mb4", "utf8mb4_latvian_ci" },
  { 227, "utf8mb4", "utf8mb4_romanian_ci" },
  { 228, "utf8mb4", "utf8mb4_slovenian_ci" },
  { 229, "utf8mb4", "utf8mb4_polish_ci" },
  { 230, "utf8mb4", "utf8mb4_estonian_ci" },
  { 231, "utf8mb4", "utf8mb4_spanish_ci" },
  { 232, "utf8mb4", "utf8mb4_swedish_ci" },
  { 233, "utf8mb4", "utf8mb4_turkish_ci" },
  { 234, "utf8mb4", "utf8mb4_czech_ci" },
  { 235, "utf8mb4", "utf8mb4_danish_ci" },
  { 236, "utf8mb4", "utf8mb4_lithuanian_ci" },
  { 237, "utf8mb4", "utf8mb4_slovak_ci" },
  { 238, "utf8mb4", "utf8mb4_spanish2_ci" },
  { 239, "utf8mb4", "utf8mb4_roman_ci" },
  { 240, "utf8mb4", "utf8mb4_persian_ci" },
  { 241, "utf8mb4", "utf8mb4_esperanto_ci" },
  { 242, "utf8mb4", "utf8mb4_hungarian_ci" },
  { 243, "utf8mb4", "utf8mb4_sinhala_ci" },
  { 244, "utf8mb4", "utf8mb4_german2_ci" },
  { 245, "utf8mb4", "utf8mb4_croatian_ci" },
  { 246, "utf8mb4", "utf8mb4_unicode_520_ci" },
  { 247, "utf8mb4", "utf8mb4_vietnamese_ci" },
  { 248, "gb18030", "gb18030_chinese_ci" },
  { 249, "gb18030", "gb18030_bin" },
  { 250, "gb18030", "gb18030_unicode_520_ci" }
};

bool ShellBaseResult::operator == (const Object_bridge &other) const
{
  return this == &other;
}

Column::Column(const std::string& schema, const std::string& table_name, const std::string& table_label, const std::string& column_name, const std::string& column_label,
       shcore::Value type, uint64_t length, bool numeric, uint64_t fractional, bool is_signed, const std::string &collation, const std::string &charset, bool padded) :
        _schema(schema), _table_name(table_name), _table_label(table_label), _column_name(column_name), _column_label(column_label), _collation(collation), _charset(charset),
       _length(length), _type(type), _fractional(fractional), _signed(is_signed), _padded(padded), _numeric(numeric)
{
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
}

bool Column::operator == (const Object_bridge &other) const
{
  return this == &other;
}
#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li schemaName: returns a String object with the name of the Schema to which this Column belongs.
 * \li tableName: returns a String object with the name of the Table to which this Column belongs.
 * \li tableLabel: returns a String object with the alias of the Table to which this Column belongs.
 * \li columnName: returns a String object with the name of the this Column.
 * \li columnLabel: returns a String object with the alias of this Column.
 * \li type: returns a Type object with the information about this Column data type.
 * \li length: returns an uint64_t value with the length in bytes of this Column.
 * \li fractionalDigits: returns an uint64_t value with the number of fractional digits on this Column (Only applies to certain data types).
 * \li numberSigned: returns an boolean value indicating wether a numeric Column is signed.
 * \li collationName: returns a String object with the collation name of the Column.
 * \li characterSetName: returns a String object with the collation name of the Column.
 * \li padded: returns an boolean value indicating wether the Column is padded.
 */
#endif
shcore::Value Column::get_member(const std::string &prop) const
{
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
  else
    ret_val = shcore::Cpp_object_bridge::get_member(prop);

  return ret_val;
}

Row::Row()
{
  add_property("length", "getLength");
  add_method("getField", boost::bind(&Row::get_field, this, _1), "field", shcore::String, NULL);
  add_method("getLength", boost::bind(&Row::get_member_method, this, _1, "getLength", "length"), NULL);
}

std::string &Row::append_descr(std::string &s_out, int indent, int UNUSED(quote_strings)) const
{
  std::string nl = (indent >= 0) ? "\n" : "";
  s_out += "[";
  for (size_t index = 0; index < value_iterators.size(); index++)
  {
    if (index > 0)
      s_out += ",";

    s_out += nl;

    if (indent >= 0)
      s_out.append((indent + 1) * 4, ' ');

    value_iterators[index]->second.append_descr(s_out, indent < 0 ? indent : indent + 1, '"');
  }

  s_out += nl;
  if (indent > 0)
    s_out.append(indent * 4, ' ');

  s_out += "]";

  return s_out;
}

void Row::append_json(shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  for (size_t index = 0; index < value_iterators.size(); index++)
    dumper.append_value(value_iterators[index]->first, value_iterators[index]->second);

  dumper.end_object();
}

std::string &Row::append_repr(std::string &s_out) const
{
  return append_descr(s_out);
}

bool Row::operator == (const Object_bridge &UNUSED(other)) const
{
  return false;
}

//! Returns the value of a field on the Row based on the field name.
#ifdef DOXYGEN_CPP
 //! \param args : Should contain the name of the field to be returned
#else
//! \param fieldName : The name of the field to be returned
#endif
#if DOXYGEN_JS
Object Row::getField(String fieldName){}
#elif DOXYGEN_PY
object Row::get_field(str fieldName){}
#endif
shcore::Value Row::get_field(const shcore::Argument_list &args)
{
  shcore::Value ret_val;
  args.ensure_count(1, "Row.getField");

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error("Row.getField: Argument #1 is expected to be a string");

  std::string field = args[0].as_string();

  if (values.find(field) != values.end())
    ret_val = values[field];
  else
    throw shcore::Exception::argument_error("Row.getField: Field " + field + " does not exist");

  return ret_val;
}

#if DOXYGEN_CPP
/**
 * Use this function to retrieve an valid member of this class exposed to the scripting languages.
 * \param prop : A string containing the name of the member to be returned
 *
 * This function returns a Value that wraps the object returned by this function. The the content of the returned value depends on the property being requested. The next list shows the valid properties as well as the returned value for each of them:
 *
 * \li length: returns the number of fields contained on this Row object.
 * \li Each field is exposed as a member of this Row object, if prop is a valid field name the value for that field will be returned.
 *
 * NOTE: if a field of on the Row is named "length", it´s value must be retrieved using the get_field() function.
 */
#else
/**
 *  Returns the number of field contained on this Row object.
 */
#if DOXYGEN_JS
Integer Row::getLength(){}
#elif DOXYGEN_PY
int Row::get_length(){}
#endif
#endif
shcore::Value Row::get_member(const std::string &prop) const
{
  if (prop == "length")
    return shcore::Value((int)values.size());
  else
  {
    std::map<std::string, shcore::Value>::const_iterator it;
    if ((it = values.find(prop)) != values.end())
      return it->second;
  }

  return shcore::Cpp_object_bridge::get_member(prop);
}

#if DOXYGEN_CPP
/**
 * Returns the value of a field on the Row based on the field position.
 */
#endif
shcore::Value Row::get_member(size_t index) const
{
  if (index < value_iterators.size())
    return value_iterators[index]->second;
  else
    return shcore::Value();
}

void Row::add_item(const std::string &key, shcore::Value value)
{
  if (shcore::is_valid_identifier(key))
    add_property(key);

  value_iterators.push_back(values.insert(values.end(), std::pair<std::string, shcore::Value>(key, value)));
}
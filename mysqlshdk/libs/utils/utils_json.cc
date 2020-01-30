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

#include "mysqlshdk/libs/utils/utils_json.h"

#include <cstring>

#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/dtoa.h"

namespace shcore {

size_t fmt_double(double d, char *buffer, size_t buffer_len) {
  return my_gcvt(d, MY_GCVT_ARG_DOUBLE, buffer_len, buffer, NULL);
}

JSON_dumper::JSON_dumper(bool pprint) {
  _deep_level = 0;

  if (pprint)
    _writer = new Pretty_writer();
  else
    _writer = new Raw_writer();
}

JSON_dumper::~JSON_dumper() {
  if (_writer) delete (_writer);
}

void JSON_dumper::append_value(const Value &value) {
  switch (value.type) {
    case Undefined:
      // TODO: Decide what to do on undefineds
      break;
    case shcore::Null:
      _writer->append_null();
      break;
    case Bool:
      _writer->append_bool(value.as_bool());
      break;
    case Integer:
      _writer->append_int64(value.as_int());
      break;
    case UInteger:
      _writer->append_uint64(value.as_uint());
      break;
    case Float:
      _writer->append_float(value.as_double());
      break;
    case String:
      _writer->append_string(value.get_string());
      break;
    case Object: {
      auto object = value.as_object();
      if (object)
        object->append_json(*this);
      else
        _writer->append_null();
    } break;
    case Array: {
      Value::Array_type_ref array = value.as_array();

      if (array) {
        Value::Array_type::const_iterator index, end = array->end();

        _writer->start_array();

        for (index = array->begin(); index != end; ++index)
          append_value(*index);

        _writer->end_array();
      } else
        _writer->append_null();
    } break;
    case Map: {
      Value::Map_type_ref map = value.as_map();

      if (map) {
        Value::Map_type::const_iterator index, end = map->end();

        _writer->start_object();

        for (index = map->begin(); index != end; ++index) {
          _writer->append_string(index->first);
          append_value(index->second);
        }

        _writer->end_object();
      } else
        _writer->append_null();
    } break;
    case MapRef:
      // TODO: define what to do with this too
      // s_out.append("mapref");
      break;
    case Function:
      value.as_function()->append_json(this);
      break;
  }
}

void JSON_dumper::append_value(const std::string &key, const Value &value) {
  _writer->append_string(key);
  append_value(value);
}

void JSON_dumper::append(const Value &value) { append_value(value); }

void JSON_dumper::append(const std::string &key, const Value &value) {
  append_value(key, value);
}

void JSON_dumper::append(const char *key, const Value &value) {
  _writer->append_string(key, std::strlen(key));
  append_value(value);
}

void JSON_dumper::append(const Dictionary_t &value) {
  append_value(Value(value));
}

void JSON_dumper::append(const std::string &key, const Dictionary_t &value) {
  append_value(key, Value(value));
}

void JSON_dumper::append(const char *key, const Dictionary_t &value) {
  append(key, Value(value));
}

void JSON_dumper::append(const Array_t &value) { append_value(Value(value)); }

void JSON_dumper::append(const std::string &key, const Array_t &value) {
  append_value(key, Value(value));
}

void JSON_dumper::append(const char *key, const Array_t &value) {
  append(key, Value(value));
}

void JSON_dumper::append_null() const { _writer->append_null(); }

void JSON_dumper::append_null(const std::string &key) const {
  _writer->append_string(key);
  _writer->append_null();
}

void JSON_dumper::append_null(const char *key) const {
  _writer->append_string(key, std::strlen(key));
  append_null();
}

void JSON_dumper::append_bool(bool data) const { _writer->append_bool(data); }

void JSON_dumper::append_bool(const std::string &key, bool data) const {
  _writer->append_string(key);
  _writer->append_bool(data);
}

void JSON_dumper::append(bool data) const { append_bool(data); }

void JSON_dumper::append(const std::string &key, bool data) const {
  append_bool(key, data);
}

void JSON_dumper::append(const char *key, bool data) const {
  _writer->append_string(key, std::strlen(key));
  append_bool(data);
}

void JSON_dumper::append_int(int data) const { _writer->append_int(data); }

void JSON_dumper::append_int(const std::string &key, int data) const {
  _writer->append_string(key);
  _writer->append_int(data);
}

void JSON_dumper::append(int data) const { append_int(data); }

void JSON_dumper::append(const std::string &key, int data) const {
  append_int(key, data);
}

void JSON_dumper::append(const char *key, int data) const {
  _writer->append_string(key, std::strlen(key));
  append_int(data);
}

void JSON_dumper::append_int64(int64_t data) const {
  _writer->append_int64(data);
}

void JSON_dumper::append_int64(const std::string &key, int64_t data) const {
  _writer->append_string(key);
  _writer->append_int64(data);
}

void JSON_dumper::append(int64_t data) const { append_int64(data); }

void JSON_dumper::append(const std::string &key, int64_t data) const {
  append_int64(key, data);
}

void JSON_dumper::append(const char *key, int64_t data) const {
  _writer->append_string(key, std::strlen(key));
  append_int64(data);
}

void JSON_dumper::append_uint(unsigned int data) const {
  _writer->append_uint(data);
}

void JSON_dumper::append_uint(const std::string &key, unsigned int data) const {
  _writer->append_string(key);
  _writer->append_uint(data);
}

void JSON_dumper::append(unsigned int data) const { append_uint(data); }

void JSON_dumper::append(const std::string &key, unsigned int data) const {
  append_uint(key, data);
}

void JSON_dumper::append(const char *key, unsigned int data) const {
  _writer->append_string(key, std::strlen(key));
  append_uint(data);
}

void JSON_dumper::append_uint64(uint64_t data) const {
  _writer->append_uint64(data);
}

void JSON_dumper::append_uint64(const std::string &key, uint64_t data) const {
  _writer->append_string(key);
  _writer->append_uint64(data);
}

void JSON_dumper::append(uint64_t data) const { append_uint64(data); }

void JSON_dumper::append(const std::string &key, uint64_t data) const {
  append_uint64(key, data);
}

void JSON_dumper::append(const char *key, uint64_t data) const {
  _writer->append_string(key, std::strlen(key));
  append_uint64(data);
}

void JSON_dumper::append_string(const std::string &data) const {
  _writer->append_string(data);
}

void JSON_dumper::append_string(const char *data, size_t length) const {
  _writer->append_string(data, length);
}

void JSON_dumper::append_string(const std::string &key,
                                const std::string &data) const {
  _writer->append_string(key);
  _writer->append_string(data);
}

void JSON_dumper::append(const std::string &data) const { append_string(data); }

void JSON_dumper::append(const std::string &key,
                         const std::string &data) const {
  append_string(key, data);
}

void JSON_dumper::append(const char *key, const std::string &data) const {
  _writer->append_string(key, std::strlen(key));
  append_string(data);
}

void JSON_dumper::append_float(double data) const {
  _writer->append_float(data);
}

void JSON_dumper::append_float(const std::string &key, double data) const {
  _writer->append_string(key);
  _writer->append_float(data);
}

void JSON_dumper::append(double data) const { append_float(data); }

void JSON_dumper::append(const std::string &key, double data) const {
  append_float(key, data);
}

void JSON_dumper::append(const char *key, double data) const {
  _writer->append_string(key, std::strlen(key));
  append_float(data);
}

void JSON_dumper::append_json(const std::string &data) const {
  rapidjson::Document document;
  document.Parse(data.c_str());
  _writer->append_document(document);
}

}  // namespace shcore

/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/types.h"
#include <rapidjson/prettywriter.h>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include "mysqlshdk/libs/utils/logger.h"
#include "utils/dtoa.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

#ifdef WIN32
#ifdef max
#undef max
#endif
#endif

// These is* functions have undefined behavior if the passed value
// is out of the -1-255 range
#define IS_ALPHA(x) (isalpha(static_cast<unsigned char>(x)))
#define IS_DIGIT(x) (isdigit(static_cast<unsigned char>(x)))

// kTypeConvertible[from_type][to_type] = is_convertible
// from_type = row, to_type = column
#define T true
#define F false
const bool shcore::kTypeConvertible[12][12] = {
    // Undf, Null,Bool,Str, Int, UInt,Flot,Obj, Arr, Map, MapR,Fun
    {T, F, F, F, F, F, F, F, F, F, F, F},  // Undefined
    {T, T, F, F, F, F, F, T, T, T, T, T},  // Null
    {T, F, T, F, T, T, T, F, F, F, F, F},  // Bool
    {T, F, T, T, T, T, T, F, F, F, F, F},  // String
    {T, F, T, F, T, T, T, F, F, F, F, F},  // Integer
    {T, F, T, F, T, T, T, F, F, F, F, F},  // UInteger
    {T, F, T, F, T, T, T, F, F, F, F, F},  // Float
    {T, F, F, F, F, F, F, T, F, F, F, F},  // Object
    {T, F, F, F, F, F, F, F, T, F, F, F},  // Array
    {T, F, F, F, F, F, F, F, F, T, T, F},  // Map
    {T, F, F, F, F, F, F, F, F, T, T, F},  // MapRef
    {T, F, F, F, F, F, F, F, F, F, F, T},  // Function
};
#undef T
#undef F
// Note: Null can be cast to Object/Array/Map, but a valid Object/Array/Map
// pointer is not NULL, so they can't be cast to it.

/**
 * Translate hex value from printable ascii character ([0-9a-zA-Z]) to decimal
 * value
 */
static const uint32_t ascii_to_hex[256] = {
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  1,  2,  3, 4, 5, 6, 7, 8, 9,  0,  0,
    0,  0,  0,  0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 10, 11, 12,
    13, 14, 15, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
    0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0};

namespace shcore {

namespace {

void skip_whitespace(const char **pc) {
  while (**pc == ' ' || **pc == '\t' || **pc == '\r' || **pc == '\n' ||
         **pc == '\v' || **pc == '\f')
    ++*pc;
}

}  // namespace

// --

Exception::Exception(const std::string &message, int code,
                     const std::shared_ptr<Value::Map_type> &e)
    : shcore::Error(message, code), _error(e) {}

Exception::Exception(const std::string &message, int code)
    : shcore::Error(message, code) {
  _error = shcore::make_dict("type", Value("MYSQLSH"), "message",
                             Value(message), "code", Value(code));
}

Exception::Exception(const std::string &type, const std::string &message,
                     int code)
    : shcore::Error(message, code) {
  _error = shcore::make_dict("type", Value(type), "message", Value(message),
                             "code", Value(code));
}

Exception Exception::argument_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ArgumentError");
  (*error)["message"] = Value(message);
  Exception e(message, -1, error);
  return e;
}

Exception Exception::attrib_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("AttributeError");
  (*error)["message"] = Value(message);
  Exception e(message, -1, error);
  return e;
}

Exception Exception::type_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("TypeError");
  (*error)["message"] = Value(message);
  Exception e(message, -1, error);
  return e;
}

Exception Exception::value_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ValueError");
  (*error)["message"] = Value(message);
  Exception e(message, -1, error);
  return e;
}

Exception Exception::logic_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("LogicError");
  (*error)["message"] = Value(message);
  Exception e(message, -1, error);
  return e;
}

Exception Exception::runtime_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("RuntimeError");
  (*error)["message"] = Value(message);
  Exception e(message, -1, error);
  return e;
}

Exception Exception::scripting_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ScriptingError");
  (*error)["message"] = Value(message);
  Exception e(message, -1, error);
  return e;
}

Exception Exception::metadata_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("MetadataError");
  (*error)["message"] = Value(message);
  Exception e(message, -1, error);
  return e;
}

Exception Exception::error_with_code(const std::string &type,
                                     const std::string &message, int code) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value(type);
  (*error)["message"] = Value(message);
  (*error)["code"] = Value(code);
  Exception e(message, code, error);
  return e;
}

Exception Exception::error_with_code_and_state(const std::string &type,
                                               const std::string &message,
                                               int code, const char *sqlstate) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value(type);
  (*error)["message"] = Value(message);
  (*error)["code"] = Value(code);
  if (sqlstate && *sqlstate) (*error)["state"] = Value(std::string(sqlstate));
  Exception e(message, code, error);
  return e;
}

Exception Exception::parser_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ParserError");
  (*error)["message"] = Value(message);
  Exception e(message, 0, error);
  return e;
}

void Exception::set_file_context(const std::string &file, size_t line) {
  if (!file.empty()) (*_error)["file"] = Value(file);
  if (line > 0) (*_error)["line"] = Value(static_cast<uint64_t>(line));
}

const char *Exception::type() const noexcept {
  if ((*_error)["type"].type == String)
    return (*_error)["type"].value.s->c_str();
  return "Exception";
}

bool Exception::is_argument() const {
  return strcmp(type(), "ArgumentError") == 0;
}

bool Exception::is_attribute() const {
  return strcmp(type(), "AttributeError") == 0;
}

bool Exception::is_value() const { return strcmp(type(), "ValueError") == 0; }

bool Exception::is_type() const { return strcmp(type(), "TypeError") == 0; }

bool Exception::is_mysql() const { return strcmp(type(), "MySQL Error") == 0; }

bool Exception::is_mysqlsh() const {
  return strcmp(type(), "MySQL Shell Error") == 0;
}

bool Exception::is_parser() const { return strcmp(type(), "ParserError") == 0; }

std::string Exception::format() const {
  std::string error_message;

  std::string type = _error->get_string("type", "");
  std::string message = what();
  std::string state = _error->get_string("state", "");
  std::string error_location = _error->get_string("location", "");

  if (!message.empty()) {
    if (!type.empty()) error_message += type;

    if (code() != -1 &&
        !is_mysqlsh()) {  // don't show shell error codes for now
      error_message += " " + std::to_string(code());
      if (!state.empty()) error_message += " (" + state + ")";
    }

    if (!error_message.empty()) error_message += ": ";

    error_message += message;

    if (!error_location.empty()) error_message += " at " + error_location;
  }

  return error_message;
}

// --

std::string type_name(Value_type type) {
  switch (type) {
    case Undefined:
      return "Undefined";
    case shcore::Null:
      return "Null";
    case Bool:
      return "Bool";
    case Integer:
      return "Integer";
    case UInteger:
      return "UInteger";
    case Float:
      return "Float";
    case String:
      return "String";
    case Object:
      return "Object";
    case Array:
      return "Array";
    case Map:
      return "Map";
    case MapRef:
      return "MapRef";
    case Function:
      return "Function";
    default:
      return "";
  }
}

std::string type_description(Value_type type) {
  switch (type) {
    case Undefined:
      return "an undefined";
    case shcore::Null:
      return "a null";
    case Bool:
      return "a bool";
    case Integer:
      return "an integer";
    case UInteger:
      return "an unsigned integer";
    case Float:
      return "a float";
    case String:
      return "a string";
    case Object:
      return "an object";
    case Array:
      return "an array";
    case Map:
      return "a map";
    case MapRef:
      return "a map";
    case Function:
      return "a function";
    default:
      return "";
  }
}

// --

Value_type Value::Map_type::get_type(const std::string &k) const {
  const_iterator iter = find(k);
  if (iter == end()) return Undefined;
  return iter->second.type;
}

std::string Value::Map_type::get_string(const std::string &k,
                                        const std::string &def) const {
  const_iterator iter = find(k);
  if (iter == end()) return def;
  iter->second.check_type(String);
  return iter->second.get_string();
}

bool Value::Map_type::get_bool(const std::string &k, bool def) const {
  const_iterator iter = find(k);
  if (iter == end()) return def;
  iter->second.check_type(Bool);
  return iter->second.as_bool();
}

int64_t Value::Map_type::get_int(const std::string &k, int64_t def) const {
  const_iterator iter = find(k);
  if (iter == end()) return def;
  iter->second.check_type(Integer);
  return iter->second.as_int();
}

uint64_t Value::Map_type::get_uint(const std::string &k, uint64_t def) const {
  const_iterator iter = find(k);
  if (iter == end()) return def;
  iter->second.check_type(UInteger);
  return iter->second.as_uint();
}

double Value::Map_type::get_double(const std::string &k, double def) const {
  const_iterator iter = find(k);
  if (iter == end()) return def;
  iter->second.check_type(Float);
  return iter->second.as_double();
}

std::shared_ptr<Value::Map_type> Value::Map_type::get_map(
    const std::string &k, std::shared_ptr<Map_type> def) const {
  const_iterator iter = find(k);
  if (iter == end()) return def;
  iter->second.check_type(Map);
  return iter->second.as_map();
}

std::shared_ptr<Value::Array_type> Value::Map_type::get_array(
    const std::string &k, std::shared_ptr<Array_type> def) const {
  const_iterator iter = find(k);
  if (iter == end()) return def;
  iter->second.check_type(Array);
  return iter->second.as_array();
}

void Value::Map_type::merge_contents(std::shared_ptr<Map_type> source,
                                     bool overwrite) {
  Value::Map_type::const_iterator iter;
  for (iter = source->begin(); iter != source->end(); ++iter) {
    std::string k = iter->first;
    Value v = iter->second;

    if (!overwrite && this->has_key(k)) continue;

    (*this)[k] = v;
  }
}

Value::Value(const Value &copy) : type(shcore::Null) { operator=(copy); }

Value::Value(const std::string &s) : type(String) {
  value.s = new std::string(s);
}

Value::Value(std::string &&s) : type(String) {
  value.s = new std::string(std::move(s));
}

Value::Value(const char *s) {
  if (s) {
    type = String;
    value.s = new std::string(s);
  } else {
    type = shcore::Null;
  }
}

Value::Value(const char *s, size_t n) {
  if (s) {
    type = String;
    value.s = new std::string(s, n);
  } else {
    type = shcore::Null;
  }
}

Value::Value(int i) : type(Integer) { value.i = i; }

Value::Value(unsigned int ui) : type(UInteger) { value.ui = ui; }

Value::Value(int64_t i) : type(Integer) { value.i = i; }

Value::Value(uint64_t ui) : type(UInteger) { value.ui = ui; }

Value::Value(float f) : type(Float) {
  // direct typecast from float to double works by just appending 0s to the
  // binary IEEE representation, which will result in a different number
  // So we convert through decimal instead
  char buffer[100];
  my_gcvt(static_cast<double>(f), MY_GCVT_ARG_FLOAT, sizeof(buffer) - 1, buffer,
          NULL);
  value.d = std::stod(buffer);
}

Value::Value(double d) : type(Float) { value.d = d; }

Value::Value(bool b) : type(Bool) { value.b = b; }

Value::Value(std::shared_ptr<Function_base> f) : type(Function) {
  if (f) {
    value.func = new std::shared_ptr<Function_base>(f);
  } else {
    type = shcore::Null;
  }
}

Value::Value(std::shared_ptr<Object_bridge> n) : type(Object) {
  if (n) {
    value.o = new std::shared_ptr<Object_bridge>(n);
  } else {
    type = shcore::Null;
  }
}

Value::Value(Map_type_ref n) : type(Map) {
  if (n) {
    value.map = new std::shared_ptr<Map_type>(n);
  } else {
    type = shcore::Null;
  }
}

Value::Value(std::weak_ptr<Map_type> n) : type(MapRef) {
  value.mapref = new std::weak_ptr<Map_type>(n);
}

Value::Value(Array_type_ref n) : type(Array) {
  if (n) {
    value.array = new std::shared_ptr<Array_type>(n);
  } else {
    type = shcore::Null;
  }
}

Value &Value::operator=(const Value &other) {
  if (type == other.type) {
    switch (type) {
      case Undefined:
        break;
      case shcore::Null:
        break;
      case Bool:
        value.b = other.value.b;
        break;
      case Integer:
        value.i = other.value.i;
        break;
      case UInteger:
        value.ui = other.value.ui;
        break;
      case Float:
        value.d = other.value.d;
        break;
      case String:
        *value.s = *other.value.s;
        break;
      case Object:
        *value.o = *other.value.o;
        break;
      case Array:
        *value.array = *other.value.array;
        break;
      case Map:
        *value.map = *other.value.map;
        break;
      case MapRef:
        *value.mapref = *other.value.mapref;
        break;
      case Function:
        *value.func = *other.value.func;
        break;
    }
  } else {
    switch (type) {
      case Undefined:
      case shcore::Null:
      case Bool:
      case Integer:
      case UInteger:
      case Float:
        break;
      case String:
        delete value.s;
        break;
      case Object:
        delete value.o;
        break;
      case Array:
        delete value.array;
        break;
      case Map:
        delete value.map;
        break;
      case MapRef:
        delete value.mapref;
        break;
      case Function:
        delete value.func;
        break;
    }
    type = other.type;
    switch (type) {
      case Undefined:
      case shcore::Null:
        break;
      case Bool:
        value.b = other.value.b;
        break;
      case Integer:
        value.i = other.value.i;
        break;
      case UInteger:
        value.ui = other.value.ui;
        break;
      case Float:
        value.d = other.value.d;
        break;
      case String:
        value.s = new std::string(*other.value.s);
        break;
      case Object:
        value.o = new std::shared_ptr<Object_bridge>(*other.value.o);
        break;
      case Array:
        value.array = new std::shared_ptr<Array_type>(*other.value.array);
        break;
      case Map:
        value.map = new std::shared_ptr<Map_type>(*other.value.map);
        break;
      case MapRef:
        value.mapref = new std::weak_ptr<Map_type>(*other.value.mapref);
        break;
      case Function:
        value.func = new std::shared_ptr<Function_base>(*other.value.func);
        break;
    }
  }
  return *this;
}

Value &Value::operator=(Value &&other) {
  switch (type) {
    case Undefined:
    case shcore::Null:
    case Bool:
    case Integer:
    case UInteger:
    case Float:
      break;
    case String:
      delete value.s;
      value.s = nullptr;
      break;
    case Object:
      delete value.o;
      value.s = nullptr;
      break;
    case Array:
      delete value.array;
      value.s = nullptr;
      break;
    case Map:
      delete value.map;
      value.s = nullptr;
      break;
    case MapRef:
      delete value.mapref;
      value.s = nullptr;
      break;
    case Function:
      delete value.func;
      value.s = nullptr;
      break;
  }

  type = Undefined;
  std::swap(type, other.type);

  switch (type) {
    case Undefined:
      break;
    case shcore::Null:
      break;
    case Bool:
      std::swap(value.b, other.value.b);
      break;
    case Integer:
      std::swap(value.i, other.value.i);
      break;
    case UInteger:
      std::swap(value.ui, other.value.ui);
      break;
    case Float:
      std::swap(value.d, other.value.d);
      break;
    case String:
      std::swap(value.s, other.value.s);
      break;
    case Object:
      std::swap(value.o, other.value.o);
      break;
    case Array:
      std::swap(value.array, other.value.array);
      break;
    case Map:
      std::swap(value.map, other.value.map);
      break;
    case MapRef:
      std::swap(value.mapref, other.value.mapref);
      break;
    case Function:
      std::swap(value.func, other.value.func);
      break;
  }
  return *this;
}

Value Value::parse_map(const char **pc) {
  Map_type_ref map(new Map_type());

  // Skips the opening {
  ++*pc;

  bool done = false;
  while (!done) {
    skip_whitespace(pc);

    if (**pc == '}') {
      ++*pc;
      break;
    }

    Value key, value;
    if (**pc != '"' && **pc != '\'')
      throw Exception::parser_error(
          "Error parsing map, unexpected character reading key.");
    else {
      key = parse_string(pc, **pc);

      skip_whitespace(pc);

      if (**pc != ':')
        throw Exception::parser_error(
            "Error parsing map, unexpected item value separator.");

      // skips the :
      ++*pc;

      skip_whitespace(pc);

      value = parse(pc);

      (*map)[key.get_string()] = value;

      skip_whitespace(pc);

      if (**pc == '}') {
        done = true;

        // Skips the }
        ++*pc;
      } else if (**pc == ',')
        ++*pc;
      else
        throw Exception::parser_error(
            "Error parsing map, unexpected item separator.");

      skip_whitespace(pc);
    }
  }

  return Value(map);
}

Value Value::parse_array(const char **pc) {
  Array_type_ref array(new Array_type());

  // Skips the opening [
  ++*pc;

  skip_whitespace(pc);

  bool done = false;
  while (!done) {
    skip_whitespace(pc);

    if (**pc != ']') array->push_back(parse(pc));

    skip_whitespace(pc);

    if (**pc == ']') {
      done = true;

      // Skips the ]
      ++*pc;
    } else if (**pc == ',')
      ++*pc;
    else
      throw Exception::parser_error(
          "Error parsing array, unexpected value separator.");

    skip_whitespace(pc);
  }

  return Value(array);
}

inline std::string unicode_codepoint_to_utf8(uint32_t uni) {
  std::string s;
  if (uni <= 0x7f) {
    s.push_back(static_cast<char>(uni & 0xff));  // 0xxxxxxx
  } else if (uni <= 0x7ff) {
    s.push_back(static_cast<char>(((uni >> 6) & 0xff) | 0xc0));  // 110xxxxxx
    s.push_back(static_cast<char>((uni & 0x3f) | 0x80));         // 10xxxxxx
  } else if (uni <= 0xffff) {
    s.push_back(static_cast<char>(((uni >> 12) & 0xff) | 0xe0));  // 1110xxxx
    s.push_back(static_cast<char>(((uni >> 6) & 0x3f) | 0x80));   // 110xxxxxx
    s.push_back(static_cast<char>((uni & 0x3f) | 0x80));          // 10xxxxxx
  } else {
    if (uni >= 0x10ffff) {
      throw Exception::parser_error("Invalid unicode codepoint");
    }
    s.push_back(static_cast<char>(((uni >> 18) & 0xff) | 0xf0));  // 11110xxx
    s.push_back(static_cast<char>(((uni >> 12) & 0x3f) | 0x80));  // 1110xxxx
    s.push_back(static_cast<char>(((uni >> 6) & 0x3f) | 0x80));   // 110xxxxxx
    s.push_back(static_cast<char>((uni & 0x3f) | 0x80));          // 10xxxxxx
  }
  return s;
}

Value Value::parse_string(const char **pc, char quote) {
  const char *p = *pc;

  // calculate length
  while (*p && *++p != quote) {
    // escaped char
    if (*p == '\\') ++p;
  }

  int32_t len = p - *pc;

  if (*p != quote) {
    std::string msg = "missing closing ";
    msg.push_back(quote);
    throw Exception::parser_error(msg);
  }

  std::string s;

  p = *pc;
  ++*pc;
  while (**pc != '\0' && (*pc - p < len)) {
    const char *pc_i = *pc;
    if (*pc_i == '\\') {
      switch (*(pc_i + 1)) {
        case 'n':
          s.append("\n");
          break;
        case '"':
          s.append("\"");
          break;
        case '\'':
          s.append("\'");
          break;
        case 'a':
          s.append("\a");
          break;
        case 'b':
          s.append("\b");
          break;
        case 'f':
          s.append("\f");
          break;
        case 'r':
          s.append("\r");
          break;
        case 't':
          s.append("\t");
          break;
        case 'v':
          s.append("\v");
          break;
        case '\\':
          s.append("\\");
          break;
        case 'x': {
          if (*pc - (pc_i + 1 + 2) < len && isxdigit(pc_i[2]) &&
              isxdigit(pc_i[3])) {
            const char c =
                (ascii_to_hex[static_cast<unsigned char>(pc_i[2])] << 4) |
                ascii_to_hex[static_cast<unsigned char>(pc_i[3])];
            s.append(std::string{c});
            pc_i += 2;
          } else {
            throw Exception::parser_error("Invalid \\xXX hex escape");
          }
        } break;
        case 'u':
          if (*pc - (pc_i + 1 + 4) < len && isxdigit(pc_i[2]) &&
              isxdigit(pc_i[3]) && isxdigit(pc_i[4]) && isxdigit(pc_i[5])) {
            uint32_t unich =
                (ascii_to_hex[static_cast<unsigned>(pc_i[2])] << 12) |
                (ascii_to_hex[static_cast<unsigned>(pc_i[3])] << 8) |
                (ascii_to_hex[static_cast<unsigned>(pc_i[4])] << 4) |
                ascii_to_hex[static_cast<unsigned>(pc_i[5])];
            s.append(unicode_codepoint_to_utf8(unich));
            pc_i += 4;
          } else {
            throw Exception::parser_error("Invalid \\uXXXX unicode escape");
          }
          break;
        case '\0':
          s.append("\0");
          break;
      }
      *pc = pc_i + 2;
    } else {
      s.append(*pc, 1);
      ++*pc;
    }
  }

  // Skips the closing quote
  ++*pc;

  return Value(s);
}

Value Value::parse_single_quoted_string(const char **pc) {
  return parse_string(pc, '\'');
}

Value Value::parse_double_quoted_string(const char **pc) {
  return parse_string(pc, '"');
}

Value Value::parse_number(const char **pcc) {
  Value ret_val;
  const char *pc = *pcc;

  // Sign can appear at the beggining
  if (*pc == '-' || *pc == '+') ++pc;

  // Continues while there are digits
  while (*pc && IS_DIGIT(*++pc)) {
  }

  bool is_integer = true;
  if (tolower(*pc) == '.') {
    is_integer = false;

    // Skips the .
    ++pc;

    // Continues while there are digits
    while (*pc && IS_DIGIT(*++pc)) {
    }
  }

  if (tolower(*pc) == 'e')  // exponential
  {
    is_integer = false;

    // Skips the e
    ++pc;

    // Sign can appear for exponential numbers
    if (*pc == '-' || *pc == '+') ++pc;

    // Continues while there are digits
    while (*pc && IS_DIGIT(*++pc))
      ;
  }

  size_t len = pc - *pcc;
  std::string number(*pcc, len);

  if (is_integer) {
    int64_t ll = 0;
    try {
      ll = std::atoll(number.c_str());
    } catch (const std::invalid_argument &e) {
      std::string s = "Error parsing int: ";
      s += e.what();
      throw Exception::parser_error(s);
    }
    ret_val = Value(static_cast<int64_t>(ll));
  } else {
    double d = 0;
    try {
      d = std::stod(number.c_str());
    } catch (const std::invalid_argument &e) {
      std::string s = "Error parsing float: ";
      s += e.what();
      throw Exception::parser_error(s);
    }

    ret_val = Value(d);
  }

  *pcc = pc;

  return ret_val;
}

Value Value::parse(const std::string &s) {
  const char *begin = s.c_str();
  const char *pc = begin;

  // ignore any white-space at the beginning of string
  skip_whitespace(&pc);

  Value tmp(parse(&pc));
  size_t parsed_length = pc - begin;

  if (parsed_length < s.size()) {
    // ensure any leftover chars are just whitespaces
    skip_whitespace(&pc);
    parsed_length = pc - begin;
    if (parsed_length < s.size())
      throw Exception::parser_error(
          "Unexpected characters left at the end of document: ..." +
          std::string(pc));
  }
  return tmp;
}

Value Value::parse(const char **pc) {
  if (**pc == '{') {
    return parse_map(pc);
  } else if (**pc == '[') {
    return parse_array(pc);
  } else if (**pc == '"') {
    return parse_double_quoted_string(pc);
  } else if (**pc == '\'') {
    return parse_single_quoted_string(pc);
  } else {
    if (IS_DIGIT(**pc) || **pc == '-' || **pc == '+')  // a number
    {
      return parse_number(pc);
    } else  // a constant between true, false, null
    {
      const char *pi = *pc;
      int n;
      while (*pc && IS_ALPHA(**pc)) ++*pc;

      n = *pc - pi;
      if (n == 9 && str_caseeq(pi, "undefined", n)) {
        return Value();
      } else if (n == 4 && str_caseeq(pi, "true", n)) {
        return Value(true);
      } else if (n == 4 && str_caseeq(pi, "null", n)) {
        return Value::Null();
      } else if (n == 5 && str_caseeq(pi, "false", n)) {
        return Value(false);
      } else {
        throw Exception::parser_error(std::string("Can't parse '") + pi + "'");
      }
    }
  }

  return Value();
}

bool Value::operator==(const Value &other) const {
  if (type == other.type) {
    switch (type) {
      case Undefined:
        return true;  // undefined == undefined is true
      case shcore::Null:
        return true;
      case Bool:
        return value.b == other.value.b;
      case Integer:
        return value.i == other.value.i;
      case UInteger:
        return value.ui == other.value.ui;
      case Float:
        return value.d == other.value.d;
      case String:
        return *value.s == *other.value.s;
      case Object:
        return **value.o == **other.value.o;
      case Array:
        return **value.array == **other.value.array;
      case Map:
        return **value.map == **other.value.map;
      case MapRef:
        return *value.mapref->lock() == *other.value.mapref->lock();
      case Function:
        return **value.func == **other.value.func;
    }
  } else {
    // with type conversion
    switch (type) {
      case Undefined:
        return false;
      case shcore::Null:
        return false;
      case Bool:
        switch (other.type) {
          case Integer:
            if (other.value.i == 1)
              return value.b == true;
            else if (other.value.i == 0)
              return value.b == false;
            return false;
          case UInteger:
            if (other.value.ui == 1)
              return value.b == true;
            else if (other.value.ui == 0)
              return value.b == false;
            return false;
          case Float:
            if (other.value.d == 1.0)
              return value.b == true;
            else if (other.value.d == 0.0)
              return value.b == false;
            return false;
          default:
            return false;
        }
      case Integer:
        switch (other.type) {
          case Bool:
            return other.operator==(*this);
          case Float:
            return value.i == (int)other.value.d &&
                   ((other.value.d - (int)other.value.d) == 0.0);
          default:
            return false;
        }
      case UInteger:
        switch (other.type) {
          case Bool:
            return other.operator==(*this);
          case Float:
            return value.ui == (unsigned int)other.value.d &&
                   ((other.value.d - (int)other.value.d) == 0.0);
          default:
            return false;
        }
      case Float:
        switch (other.type) {
          case Bool:
            return other.operator==(*this);
          case Integer:
          case UInteger:
            return other.operator==(*this);
          default:
            return false;
        }
      default:
        return false;
    }
  }
  return false;
}

std::string Value::json(bool pprint) const {
  std::stringstream s;
  JSON_dumper dumper(pprint);

  dumper.append_value(*this);

  return dumper.str();
}

std::string Value::descr(bool pprint) const {
  std::string s;
  append_descr(s, pprint ? 0 : -1, false);  // top level strings are not quoted
  return s;
}

std::string Value::repr() const {
  std::string s;
  append_repr(s);
  return s;
}

std::string &Value::append_descr(std::string &s_out, int indent,
                                 char quote_strings) const {
  std::string nl = (indent >= 0) ? "\n" : "";
  switch (type) {
    case Undefined:
      s_out.append("undefined");
      break;
    case shcore::Null:
      s_out.append("null");
      break;
    case Bool:
      if (value.b)
        s_out += "true";
      else
        s_out += "false";
      break;
    case Integer:
      s_out += std::to_string(value.i);
      break;
    case UInteger:
      s_out += std::to_string(value.ui);
      break;
    case Float: {
      char buffer[32];
      size_t len;
      len = my_gcvt(value.d, MY_GCVT_ARG_DOUBLE, sizeof(buffer) - 1, buffer,
                    NULL);
      s_out.append(buffer, len);
      break;
    }
    case String:
      if (quote_strings) {
        s_out += quote_string(*value.s, quote_strings);
      } else {
        s_out += *value.s;
      }
      break;
    case Object:
      if (!value.o || !*value.o)
        throw Exception::value_error("Invalid object value encountered");
      as_object()->append_descr(s_out, indent, quote_strings);
      break;
    case Array: {
      if (!value.array || !*value.array)
        throw Exception::value_error("Invalid array value encountered");
      Array_type *vec = value.array->get();
      Array_type::iterator myend = vec->end(), mybegin = vec->begin();
      s_out += "[";
      for (Array_type::iterator iter = mybegin; iter != myend; ++iter) {
        if (iter != mybegin) s_out += ", ";
        s_out += nl;
        if (indent >= 0) s_out.append((indent + 1) * 4, ' ');
        iter->append_descr(s_out, indent < 0 ? indent : indent + 1, '"');
      }

      if (!vec->empty()) {
        s_out += nl;
        if (indent > 0) s_out.append(indent * 4, ' ');
      }

      s_out += "]";
    } break;
    case Map: {
      if (!value.map || !*value.map)
        throw Exception::value_error("Invalid map value encountered");
      Map_type *map = value.map->get();
      Map_type::iterator myend = map->end(), mybegin = map->begin();
      s_out += "{";

      if (!map->empty()) s_out += nl;

      for (Map_type::iterator iter = mybegin; iter != myend; ++iter) {
        if (iter != mybegin) s_out += ", " + nl;
        if (indent >= 0) s_out.append((indent + 1) * 4, ' ');
        s_out += quote_string(iter->first, '"') + ": ";
        iter->second.append_descr(s_out, indent < 0 ? indent : indent + 1, '"');
      }

      if (!map->empty()) {
        s_out += nl;
        if (indent > 0) s_out.append(indent * 4, ' ');
      }

      s_out += "}";
    } break;
    case MapRef:
      s_out.append("mapref");
      break;
    case Function:
      value.func->get()->append_descr(&s_out, indent, quote_strings);
      break;
  }
  return s_out;
}

std::string &Value::append_repr(std::string &s_out) const {
  switch (type) {
    case Undefined:
      s_out.append("undefined");
      break;
    case shcore::Null:
      s_out.append("null");
      break;
    case Bool:
      if (value.b)
        s_out += "true";
      else
        s_out += "false";
      break;
    case Integer: {
      std::ostringstream value_i;
      value_i << value.i;
      s_out += value_i.str();
    } break;
    case UInteger: {
      std::ostringstream value_ui;
      value_ui << value.ui;
      s_out += value_ui.str();
    } break;
    case Float: {
      s_out += str_format("%g", value.d);
    } break;
    case String: {
      std::string &s = *value.s;
      s_out += "\"";
      for (size_t i = 0; i < s.length(); i++) {
        unsigned char c = s[i];

        // non-printable ascii char
        if (!(0x20 <= c && c < 0x7f)) {
          s_out += "\\x";
          s_out += "0123456789abcdef"[c >> 4];
          s_out += "0123456789abcdef"[c & 0xf];
          continue;
        }

        switch (c) {
          case '\n':
            s_out += "\\n";
            break;
          case '"':
            s_out += "\\\"";
            break;
          case '\'':
            s_out += "\\\'";
            break;
          case '\a':
            s_out += "\\a";
            break;
          case '\b':
            s_out += "\\b";
            break;
          case '\f':
            s_out += "\\f";
            break;
          case '\r':
            s_out += "\\r";
            break;
          case '\t':
            s_out += "\\t";
            break;
          case '\v':
            s_out += "\\v";
            break;
          case '\\':
            s_out += "\\\\";
            break;
          default:
            s_out += c;
        }
      }
      s_out += "\"";
    } break;
    case Object:
      s_out = (*value.o)->append_repr(s_out);
      break;
    case Array: {
      Array_type *vec = value.array->get();
      Array_type::iterator myend = vec->end(), mybegin = vec->begin();
      s_out += "[";
      for (Array_type::iterator iter = mybegin; iter != myend; ++iter) {
        if (iter != mybegin) s_out += ", ";
        iter->append_repr(s_out);
      }
      s_out += "]";
    } break;
    case Map: {
      Map_type *map = value.map->get();
      Map_type::iterator myend = map->end(), mybegin = map->begin();
      s_out += "{";
      for (Map_type::iterator iter = mybegin; iter != myend; ++iter) {
        if (iter != mybegin) s_out += ", ";
        Value key(iter->first);
        key.append_repr(s_out);
        s_out += ": ";
        iter->second.append_repr(s_out);
      }
      s_out += "}";
    } break;
    case MapRef:
      break;
    case Function:
      value.func->get()->append_repr(&s_out);
      break;
  }
  return s_out;
}

Value::~Value() {
  switch (type) {
    case Undefined:
    case shcore::Null:
    case Bool:
    case Integer:
    case UInteger:
    case Float:
      break;
    case String:
      delete value.s;
      break;
    case Object:
      delete value.o;
      break;
    case Array:
      delete value.array;
      break;
    case Map:
      delete value.map;
      break;
    case MapRef:
      delete value.mapref;
      break;
    case Function:
      delete value.func;
      break;
  }
}

inline Exception type_conversion_error(Value_type from, Value_type expected) {
  return Exception::type_error("Invalid typecast: " + type_name(expected) +
                               " expected, but value is " + type_name(from));
}

inline Exception type_range_error(Value_type from, Value_type expected) {
  return Exception::type_error("Invalid typecast: " + type_name(expected) +
                               " expected, but " + type_name(from) +
                               " value is out of range");
}

bool is_compatible_type(Value_type source_type, Value_type target_type) {
  return kTypeConvertible[source_type][target_type];
}

void Value::check_type(Value_type t) const {
  if (!is_compatible_type(type, t)) throw type_conversion_error(type, t);
}

bool Value::as_bool() const {
  switch (type) {
    case Bool:
      return value.b;
    case Integer:
      return value.i != 0;
    case UInteger:
      return value.ui != 0;
    case Float:
      return value.d != 0.0;
    case String:
      try {
        return lexical_cast<bool>(*value.s);
      } catch (...) {
      }
      break;
    default:
      break;
  }
  throw type_conversion_error(type, Bool);
}

int64_t Value::as_int() const {
  switch (type) {
    case Integer:
      return value.i;
    case UInteger:
      if (value.ui <= (uint64_t)std::numeric_limits<int64_t>::max())
        return static_cast<int64_t>(value.ui);
      throw type_range_error(type, Integer);
    case Float: {
      double integral;
      if (std::modf(value.d, &integral) == 0.0 &&
          value.d >= -(1LL << DBL_MANT_DIG) && value.d <= (1LL << DBL_MANT_DIG))
        return static_cast<int64_t>(value.d);
      throw type_range_error(type, Integer);
    }
    case Bool:
      return value.b ? 1 : 0;
    case String:
      try {
        return lexical_cast<int64_t>(*value.s);
      } catch (...) {
      }
      break;
    default:
      break;
  }
  throw type_conversion_error(type, Integer);
}

uint64_t Value::as_uint() const {
  switch (type) {
    case UInteger:
      return value.ui;
    case Integer:
      if (value.i >= 0) return static_cast<uint64_t>(value.i);
      throw type_range_error(type, UInteger);
    case Float: {
      double integral;
      if (std::modf(value.d, &integral) == 0.0 && value.d >= 0.0 &&
          value.d <= (1LL << DBL_MANT_DIG))
        return static_cast<uint64_t>(value.d);
      throw type_range_error(type, UInteger);
    }
    case Bool:
      return value.b ? 1 : 0;
    case String:
      try {
        return lexical_cast<uint64_t>(*value.s);
      } catch (...) {
      }
      break;
    default:
      break;
  }
  throw type_conversion_error(type, UInteger);
}

double Value::as_double() const {
  // DBL_MANT_DIG is the number of bits used to represent the number itself,
  // without the exponent or sign
  switch (type) {
    case UInteger:
      if (value.ui <= (1ULL << DBL_MANT_DIG))
        return static_cast<double>(value.ui);
      // higher values lose precision
      throw type_range_error(type, Float);
    case Integer:
      if (value.i <= (1LL << DBL_MANT_DIG) && value.i >= -(1LL << DBL_MANT_DIG))
        return static_cast<double>(value.i);
      // higher or lower values lose precision
      throw type_range_error(type, Float);
    case Float:
      return value.d;
    case Bool:
      return value.b ? 1.0 : 0.0;
    case String:
      try {
        return lexical_cast<double>(*value.s);
      } catch (...) {
      }
      break;
    default:
      break;
  }
  throw type_conversion_error(type, Float);
}

std::string Value::as_string() const {
  switch (type) {
    case UInteger:
      return std::to_string(value.ui);
    case Integer:
      return std::to_string(value.i);
    case Float:
      return lexical_cast<std::string>(value.d);
    case Bool:
      return lexical_cast<std::string>(value.b);
    case String:
      return *value.s;
    default:
      break;
  }
  throw type_conversion_error(type, String);
}

std::string Value::yaml() const { return "---\n" + yaml(0) + "\n"; }

std::string Value::yaml(int init_indent) const {
  // implementation based on: http://yaml.org/spec/1.2/spec.html
  static constexpr size_t k_indent_size = 2;

  const auto new_line = [](std::string *s, int indent) {
    s->append(1, '\n');
    s->append(k_indent_size * indent, ' ');
  };

  const auto map2yaml = [&](const Dictionary_t &m, int indent) {
    if (!m) {
      // null -> empty string
      return std::string{};
    }

    std::string map;
    bool first_item = true;

    for (const auto &kv : *m) {
      if (first_item) {
        first_item = false;
      } else {
        new_line(&map, indent);
      }

      map += kv.first + ":";
      const auto sub_yaml = kv.second.yaml(indent + 1);

      // if value is a map or an array it needs to be in a separate line
      if (kv.second.type == Value_type::Map ||
          kv.second.type == Value_type::MapRef ||
          kv.second.type == Value_type::Array) {
        new_line(&map, indent + 1);
      } else {
        if (!sub_yaml.empty()) {
          map += " ";
        }
      }

      map += sub_yaml;
    }

    return map;
  };

  const auto string2yaml = [](const std::string &s, int indent) -> std::string {
    if (s.empty()) {
      // empty string must be explicitly quoted
      return "''";
    }

    auto content = split_string(s, "\n");

    if (content.size() == 1) {
      // use either plain or quoted style
      bool quote = false;

      {
        // plain string cannot contain leading or trailing white-space
        // characters
        static constexpr auto k_whitespace = " \t";
        if (std::string{k_whitespace}.find(s[0]) != std::string::npos ||
            std::string{k_whitespace}.find(s.back()) != std::string::npos) {
          quote = true;
        }
      }

      if (!quote) {
        // plain string cannot start with an indicator
        if (std::string{"-?:,[]{}#&*!|>'\"%@`"}.find(s[0]) !=
            std::string::npos) {
          if (s.length() > 1 &&
              std::string{"-?:"}.find(s[0]) != std::string::npos &&
              s[1] != ' ') {
            // unless it's '-', '?' or ':' and it's not followed by a space
            quote = false;
          } else {
            quote = true;
          }
        }
      }

      if (!quote) {
        // plain scalars must never contain the ": " and " #" character
        // combinations
        if (s.find(": ") != std::string::npos ||
            s.find(" #") != std::string::npos) {
          quote = true;
        }
      }

      if (quote) {
        return "'" + str_replace(s, "'", "''") + "'";
      } else {
        return s;
      }
    } else {
      // by default set chomping to 'strip'
      char chomping = '-';

      if (content.back().empty()) {
        // if the last line is an empty string, it means that the input string
        // ends with a new line character and the extra line is not needed
        content.pop_back();
        // chomping needs to be set to 'clip', so the final line break is
        // included
        chomping = 0;
      }

      if (content.back().empty()) {
        // if the last line is still an empty string, set chomping to 'keep',
        // so multiple empty lines are preserved
        chomping = '+';
      }

      // use literal block style
      std::string literal = "|";
      // increase the indent for contents
      ++indent;

      if (content[0][0] == ' ' || content[0][0] == '\t') {
        // if the first line starts with a white-space, we need to explicitly
        // specify the indent
        literal += std::to_string(k_indent_size * indent);
      }

      // add chomping indicator if not 'clip'
      if (chomping) {
        literal += chomping;
      }

      literal += "\n";

      for (const auto &line : content) {
        literal.append(k_indent_size * indent, ' ');
        literal += line;
        literal.append(1, '\n');
      }

      // remove the last new line character
      literal.pop_back();

      return literal;
    }
  };

  switch (type) {
    case Value_type::Undefined:
    case Value_type::Null:
      // YAML has no notion of these values, they are represented by empty
      // strings
      return "";

    case Value_type::Bool:
    case Value_type::Integer:
    case Value_type::UInteger:
    case Value_type::Float:
    case Value_type::Object:
    case Value_type::Function:
      // we treat these as scalars
      return string2yaml(descr(), init_indent);

    case Value_type::String:
      return string2yaml(*value.s, init_indent);

    case Value_type::Array: {
      std::string array;
      bool first_item = true;

      for (const auto &v : **value.array) {
        if (first_item) {
          first_item = false;
        } else {
          new_line(&array, init_indent);
        }

        array += "-";
        const auto sub_yaml = v.yaml(init_indent + 1);

        if (!sub_yaml.empty()) {
          array += " " + sub_yaml;
        }
      }

      return array;
    }

    case Value_type::Map:
      return map2yaml(*value.map, init_indent);

    case Value_type::MapRef:
      return map2yaml(value.mapref->lock(), init_indent);
  }

  throw std::logic_error("Type '" + type_name(type) + "' was not handled.");
}

//---

const std::string &Argument_list::string_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  switch (at(i).type) {
    case String:
      return *at(i).value.s;
    default:
      throw Exception::type_error(
          str_format("Argument #%u is expected to be a string", (i + 1)));
  }
}

bool Argument_list::bool_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  try {
    return at(i).as_bool();
  } catch (...) {
    throw Exception::type_error(
        str_format("Argument #%u is expected to be a bool", (i + 1)));
  }
}

int64_t Argument_list::int_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  try {
    return at(i).as_int();
  } catch (...) {
    throw Exception::type_error(
        str_format("Argument #%u is expected to be an int", (i + 1)));
  }
}

uint64_t Argument_list::uint_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  try {
    return at(i).as_uint();
  } catch (...) {
    throw Exception::type_error(
        str_format("Argument #%u is expected to be an unsigned int", (i + 1)));
  }
}

double Argument_list::double_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");

  try {
    return at(i).as_double();
  } catch (...) {
    throw Exception::type_error(
        str_format("Argument #%u is expected to be a double", (i + 1)));
  }
}

std::shared_ptr<Object_bridge> Argument_list::object_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Object)
    throw Exception::type_error(
        str_format("Argument #%u is expected to be an object", (i + 1)));
  return *at(i).value.o;
}

std::shared_ptr<Value::Map_type> Argument_list::map_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Map)
    throw Exception::type_error(
        str_format("Argument #%u is expected to be a map", (i + 1)));
  return *at(i).value.map;
}

std::shared_ptr<Value::Array_type> Argument_list::array_at(
    unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Array)
    throw Exception::type_error(
        str_format("Argument #%u is expected to be an array", (i + 1)));
  return *at(i).value.array;
}

void Argument_list::ensure_count(unsigned int c, const char *context) const {
  if (c != size())
    throw Exception::argument_error(
        str_format("%s: Invalid number of arguments, expected %u but got %u",
                   context, c, static_cast<uint32_t>(size())));
}

void Argument_list::ensure_count(unsigned int minc, unsigned int maxc,
                                 const char *context) const {
  if (size() < minc || size() > maxc)
    throw Exception::argument_error(str_format(
        "%s: Invalid number of arguments, expected %u to %u but got %u",
        context, minc, maxc, static_cast<uint32_t>(size())));
}

void Argument_list::ensure_at_least(unsigned int minc,
                                    const char *context) const {
  if (size() < minc)
    throw Exception::argument_error(str_format(
        "%s: Invalid number of arguments, expected at least %u but got %u",
        context, minc, static_cast<uint32_t>(size())));
}

bool Argument_list::operator==(const Argument_list &other) const {
  return _args == other._args;
}

//--

Argument_map::Argument_map() {}

Argument_map::Argument_map(const Value::Map_type &map) : _map(map) {}

const std::string &Argument_map::string_at(const std::string &key) const {
  const Value &v(at(key));
  switch (v.type) {
    case String:
      return *v.value.s;
    default:
      throw Exception::type_error(std::string("Argument ")
                                      .append(key)
                                      .append(" is expected to be a string"));
  }
}

bool Argument_map::bool_at(const std::string &key) const {
  try {
    return at(key).as_bool();
  } catch (...) {
    throw Exception::type_error(
        std::string("Argument '" + key + "' is expected to be a bool"));
  }
}

int64_t Argument_map::int_at(const std::string &key) const {
  try {
    return at(key).as_int();
  } catch (...) {
    throw Exception::type_error("Argument '" + key +
                                "' is expected to be an int");
  }
}

uint64_t Argument_map::uint_at(const std::string &key) const {
  try {
    return at(key).as_uint();
  } catch (...) {
    throw Exception::type_error("Argument '" + key +
                                "' is expected to be an unsigned int");
  }
}

double Argument_map::double_at(const std::string &key) const {
  try {
    return at(key).as_double();
  } catch (...) {
    throw Exception::type_error("Argument '" + key +
                                "' is expected to be a double");
  }
}

std::shared_ptr<Object_bridge> Argument_map::object_at(
    const std::string &key) const {
  const Value &value(at(key));
  if (value.type != Object)
    throw Exception::type_error("Argument '" + key +
                                "' is expected to be an object");
  return *value.value.o;
}

std::shared_ptr<Value::Map_type> Argument_map::map_at(
    const std::string &key) const {
  const Value &value(at(key));
  if (value.type != Map)
    throw Exception::type_error("Argument '" + key +
                                "' is expected to be a map");
  return *value.value.map;
}

std::shared_ptr<Value::Array_type> Argument_map::array_at(
    const std::string &key) const {
  const Value &value(at(key));
  if (value.type != Array)
    throw Exception::type_error("Argument '" + key +
                                "' is expected to be an array");
  return *value.value.array;
}

bool Argument_map::comp(const std::string &lhs, const std::string &rhs) {
  return lhs.compare(rhs) < 0;
}

bool Argument_map::icomp(const std::string &lhs, const std::string &rhs) {
  return shcore::str_casecmp(lhs.c_str(), rhs.c_str()) < 0;
}

void Argument_map::ensure_keys(const std::set<std::string> &mandatory_keys,
                               const std::set<std::string> &optional_keys,
                               const char *context, bool case_sensitive) const {
  std::vector<std::string> missing_keys;
  std::vector<std::string> invalid_keys;

  if (!validate_keys(mandatory_keys, optional_keys, missing_keys, invalid_keys,
                     case_sensitive)) {
    std::string msg;
    if (!invalid_keys.empty() && !missing_keys.empty()) {
      msg.append("Invalid and missing values in ").append(context).append(" ");
      msg.append("(invalid: ").append(str_join(invalid_keys, ", "));
      msg.append("), (missing: ").append(str_join(missing_keys, ", "));
      msg.append(")");
    } else if (!invalid_keys.empty()) {
      msg.append("Invalid values in ").append(context).append(": ");
      msg.append(str_join(invalid_keys, ", "));
    } else if (!missing_keys.empty()) {
      msg.append("Missing values in ").append(context).append(": ");
      msg.append(str_join(missing_keys, ", "));
    }
    if (!msg.empty()) throw Exception::argument_error(msg);
  }
}

bool Argument_map::validate_keys(const std::set<std::string> &mandatory_keys,
                                 const std::set<std::string> &optional_keys,
                                 std::vector<std::string> &missing_keys,
                                 std::vector<std::string> &invalid_keys,
                                 bool case_sensitive) const {
  std::map<std::string, std::string,
           bool (*)(const std::string &, const std::string &)>
      mandatory_aliases(case_sensitive ? Argument_map::comp
                                       : Argument_map::icomp);

  missing_keys.clear();
  invalid_keys.clear();

  std::set<std::string, bool (*)(const std::string &, const std::string &)>
      optional(case_sensitive ? Argument_map::comp : Argument_map::icomp);

  optional.insert(optional_keys.begin(), optional_keys.end());

  for (auto key : mandatory_keys) {
    auto aliases = split_string(key, "|");
    missing_keys.push_back(aliases[0]);

    for (auto alias : aliases) mandatory_aliases[alias] = aliases[0];
  }

  for (auto k : _map) {
    if (mandatory_aliases.find(k.first) != mandatory_aliases.end()) {
      auto position = std::find(missing_keys.begin(), missing_keys.end(),
                                mandatory_aliases[k.first]);
      if (position != missing_keys.end())
        missing_keys.erase(position);
      else
        // The same option was specified with two different aliases
        // Only the first is considered valid
        invalid_keys.push_back(k.first);
    } else if (optional.find(k.first) != optional.end()) {
      // nop
    } else
      invalid_keys.push_back(k.first);
  }

  return missing_keys.empty() && invalid_keys.empty();
}

Option_unpacker::Option_unpacker(const Dictionary_t &options)
    : m_options(options) {
  if (m_options)
    for (const auto &opt : *m_options) m_unknown.insert(opt.first);
}

Value Option_unpacker::get_required(const char *name, Value_type type) {
  if (!m_options) {
    m_missing.insert(name);
    return Value();
  }
  auto opt = m_options->find(name);
  if (opt == m_options->end()) {
    m_missing.insert(name);
    return Value();
  } else {
    m_unknown.erase(name);

    if (type != Undefined && !is_compatible_type(opt->second.type, type)) {
      throw Exception::type_error(str_format(
          "Option '%s' is expected to be of type %s, but is %s", name,
          type_name(type).c_str(), type_name(opt->second.type).c_str()));
    }

    return opt->second;
  }
}

Value Option_unpacker::get_optional(const char *name, Value_type type,
                                    bool case_insensitive) {
  if (!m_options) {
    return Value();
  }
  auto opt = m_options->find(name);

  if (case_insensitive && opt == m_options->end()) {
    for (auto it = m_options->begin(); it != m_options->end(); ++it) {
      if (str_caseeq(it->first.c_str(), name)) {
        name = it->first.c_str();
        opt = it;
        break;
      }
    }
  }
  if (opt != m_options->end()) {
    m_unknown.erase(name);

    if (type != Undefined && !is_compatible_type(opt->second.type, type)) {
      throw Exception::type_error(str_format(
          "Option '%s' is expected to be of type %s, but is %s", name,
          type_name(type).c_str(), type_name(opt->second.type).c_str()));
    }

    return opt->second;
  }
  return Value();
}

Value Option_unpacker::get_optional_exact(const char *name, Value_type type,
                                          bool case_insensitive) {
  if (!m_options) {
    return Value();
  }
  auto opt = m_options->find(name);

  if (case_insensitive && opt == m_options->end()) {
    for (auto it = m_options->begin(); it != m_options->end(); ++it) {
      if (str_caseeq(it->first.c_str(), name)) {
        name = it->first.c_str();
        opt = it;
        break;
      }
    }
  }
  if (opt != m_options->end()) {
    m_unknown.erase(name);
    if (type != Undefined &&
        ((opt->second.type == String && !is_compatible_type(String, type)) ||
         (opt->second.type != String && opt->second.type != type))) {
      throw Exception::type_error(str_format(
          "Option '%s' is expected to be of type %s, but is %s", name,
          type_name(type).c_str(), type_name(opt->second.type).c_str()));
    }

    return opt->second;
  }
  return Value();
}

void Option_unpacker::end(const std::string &context) { validate(context); }

void Option_unpacker::validate(const std::string &context) {
  std::string msg;
  if (!m_unknown.empty() && !m_missing.empty()) {
    msg.append("Invalid and missing options ");
    if (!context.empty()) msg.append(context + " ");
    msg.append("(invalid: ").append(str_join(m_unknown, ", "));
    msg.append("), (missing: ").append(str_join(m_missing, ", "));
    msg.append(")");
  } else if (!m_unknown.empty()) {
    msg.append("Invalid options");
    if (!context.empty()) msg.append(" " + context);
    msg.append(": ");
    msg.append(str_join(m_unknown, ", "));
  } else if (!m_missing.empty()) {
    msg.append("Missing required options");
    if (!context.empty()) msg.append(" " + context);
    msg.append(": ");
    msg.append(str_join(m_missing, ", "));
  }
  if (!msg.empty()) throw Exception::argument_error(msg);
}

//--

void Object_bridge::append_json(JSON_dumper &dumper) const {
  dumper.start_object();
  dumper.append_string("class", class_name());
  dumper.end_object();
}

std::string &Function_base::append_descr(std::string *s_out, int /* indent */,
                                         int /* quote_strings */) const {
  const auto &n = name();
  s_out->append("<Function").append(n.empty() ? "" : ":" + n).append(">");
  return *s_out;
}

std::string &Function_base::append_repr(std::string *s_out) const {
  return append_descr(s_out);
}

void Function_base::append_json(JSON_dumper *dumper) const {
  std::string repr;
  dumper->append_string(append_repr(&repr));
}

}  // namespace shcore

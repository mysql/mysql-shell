/*
 * Copyright (c) 2014, 2016 Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/types.h"
#include "utils/utils_general.h"
#include <stdexcept>
#include <cstdarg>
#include <boost/format.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <rapidjson/prettywriter.h>
#include <limits>

// These is* functions have undefined behavior if the passed value
// is out of the -1-255 range
#define IS_ALPHA(x) (isalpha(static_cast<unsigned char>(x)))
#define IS_DIGIT(x) (isdigit(static_cast<unsigned char>(x)))

using namespace shcore;

// --

Exception::Exception(const std::shared_ptr<Value::Map_type> e)
  : _error(e) {
  log_error("%s", what());
}

Exception Exception::argument_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ArgumentError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

Exception Exception::attrib_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("AttributeError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

Exception Exception::type_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("TypeError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

Exception Exception::value_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ValueError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

Exception Exception::logic_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("LogicError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

Exception Exception::runtime_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("RuntimeError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

Exception Exception::scripting_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ScriptingError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

Exception Exception::metadata_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("MetadataError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

Exception Exception::error_with_code(const std::string &type, const std::string &message, int code) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value(type);
  (*error)["message"] = Value(message);
  (*error)["code"] = Value(code);
  Exception e(error);
  return e;
}

Exception Exception::error_with_code_and_state(const std::string &type, const std::string &message, int code, const char *sqlstate) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value(type);
  (*error)["message"] = Value(message);
  (*error)["code"] = Value(code);
  (*error)["state"] = Value(std::string(sqlstate));
  Exception e(error);
  return e;
}

Exception Exception::parser_error(const std::string &message) {
  std::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ParserError");
  (*error)["message"] = Value(message);
  Exception e(error);
  return e;
}

const char *Exception::what() const BOOST_NOEXCEPT_OR_NOTHROW
{
  if ((*_error)["message"].type == String)
  return (*_error)["message"].value.s->c_str();
  return "?";
}

const char *Exception::type() const BOOST_NOEXCEPT_OR_NOTHROW
{
  if ((*_error)["type"].type == String)
  return (*_error)["type"].value.s->c_str();
  return "Exception";
}

int64_t Exception::code() const BOOST_NOEXCEPT_OR_NOTHROW
{
  if ((*_error).find("code") != (*_error).end()) {
    try {
      return (*_error)["code"].as_int();
    } catch (...) {
      return 0; //as it's not int
    }
  }
  return 0;
}

bool Exception::is_argument() const {
  return strcmp(type(), "ArgumentError") == 0;
}

bool Exception::is_attribute() const {
  return strcmp(type(), "AttributeError") == 0;
}

bool Exception::is_value() const {
  return strcmp(type(), "ValueError") == 0;
}

bool Exception::is_type() const {
  return strcmp(type(), "TypeError") == 0;
}

bool Exception::is_server() const {
  return strcmp(type(), "Server") == 0;
}

bool Exception::is_mysql() const {
  return strcmp(type(), "MySQL Error") == 0;
}

bool Exception::is_parser() const {
  return strcmp(type(), "ParserError") == 0;
}

std::string Exception::format() {
  std::string error_message;

  std::string type = _error->get_string("type", "");
  std::string message = _error->get_string("message", "");
  int64_t code = _error->get_int("code", -1);
  std::string error_location = _error->get_string("location", "");

  if (!message.empty()) {
    if (!type.empty())
      error_message += type;

    if (code != -1)
      error_message += (boost::format(" (%1%)") % code).str();

    if (!error_message.empty())
      error_message += ": ";

    error_message += message;

    if (!error_location.empty())
      error_message += " at " + error_location;

    error_message += "\n";
  }

  return error_message;
}

// --

std::string shcore::type_name(Value_type type) {
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

// --

Value_type Value::Map_type::get_type(const std::string &k) const {
  const_iterator iter = find(k);
  if (iter == end())
    return Undefined;
  return iter->second.type;
}

std::string Value::Map_type::get_string(const std::string &k, const std::string &def) const {
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(String);
  return iter->second.as_string();
}

bool Value::Map_type::get_bool(const std::string &k, bool def) const {
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Bool);
  return iter->second.as_bool();
}

int64_t Value::Map_type::get_int(const std::string &k, int64_t def) const {
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Integer);
  return iter->second.as_int();
}

uint64_t Value::Map_type::get_uint(const std::string &k, uint64_t def) const {
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(UInteger);
  return iter->second.as_uint();
}

double Value::Map_type::get_double(const std::string &k, double def) const {
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Float);
  return iter->second.as_double();
}

std::shared_ptr<Value::Map_type> Value::Map_type::get_map(const std::string &k,
  std::shared_ptr<Map_type> def) const {
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Map);
  return iter->second.as_map();
}

std::shared_ptr<Value::Array_type> Value::Map_type::get_array(const std::string &k,
  std::shared_ptr<Array_type> def) const {
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Array);
  return iter->second.as_array();
}

void Value::Map_type::merge_contents(std::shared_ptr<Map_type> source, bool overwrite) {
  Value::Map_type::const_iterator iter;
  for (iter = source->begin(); iter != source->end(); ++iter) {
    std::string k = iter->first;
    Value v = iter->second;

    if (!overwrite && this->has_key(k))
      continue;

    (*this)[k] = v;
  }
}

Value::Value(const Value &copy)
  : type(shcore::Null) {
  operator=(copy);
}

Value::Value(const std::string &s)
  : type(String) {
  value.s = new std::string(s);
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

Value::Value(int i)
  : type(Integer) {
  value.i = i;
}

Value::Value(int64_t i)
  : type(Integer) {
  value.i = i;
}

Value::Value(uint64_t ui)
  : type(UInteger) {
  value.ui = ui;
}

Value::Value(double d)
  : type(Float) {
  value.d = d;
}

Value::Value(bool b)
  : type(Bool) {
  value.b = b;
}

Value::Value(std::shared_ptr<Function_base> f)
  : type(Function) {
  value.func = new std::shared_ptr<Function_base>(f);
}

Value::Value(std::shared_ptr<Object_bridge> n)
  : type(Object) {
  value.o = new std::shared_ptr<Object_bridge>(n);
}

Value::Value(Map_type_ref n)
  : type(Map) {
  value.map = new std::shared_ptr<Map_type>(n);
}

Value::Value(std::weak_ptr<Map_type> n)
  : type(MapRef) {
  value.mapref = new std::weak_ptr<Map_type>(n);
}

Value::Value(Array_type_ref n)
  : type(Array) {
  value.array = new std::shared_ptr<Array_type>(n);
}

Value &Value::operator= (const Value &other) {
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

Value Value::parse_map(char **pc) {
  Map_type_ref map(new Map_type());

  // Skips the opening {
  ++*pc;

  bool done = false;
  while (!done) {
    // Skips the spaces
    while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;

    if (**pc == '}')
      break;

    Value key, value;
    if (**pc != '"' && **pc != '\'')
      throw Exception::parser_error("Error parsing map, unexpected character reading key.");
    else {
      key = parse_string(pc, **pc);

      // Skips the spaces
      while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;

      if (**pc != ':')
        throw Exception::parser_error("Error parsing map, unexpected item value separator.");

      // skips the :
      ++*pc;

      // Skips the spaces
      while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;

      value = parse(pc);

      (*map)[key.as_string()] = value;

      // Skips the spaces
      while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;

      if (**pc == '}') {
        done = true;

        // Skips the }
        ++*pc;
      } else if (**pc == ',')
        ++*pc;
      else
        throw Exception::parser_error("Error parsing map, unexpected item separator.");

      // Skips the spaces
      while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;
    }
  }

  return Value(map);
}

Value Value::parse_array(char **pc) {
  Array_type_ref array(new Array_type());

  // Skips the opening [
  ++*pc;

  // Skips the spaces
  while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;

  bool done = false;
  while (!done) {
    // Skips the spaces
    while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;

    if (**pc != ']')
      array->push_back(parse(pc));

    // Skips the spaces
    while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;

    if (**pc == ']') {
      done = true;

      // Skips the ]
      ++*pc;
    } else if (**pc == ',')
      ++*pc;
    else
      throw Exception::parser_error("Error parsing array, unexpected value separator.");

    // Skips the spaces
    while (**pc == ' ' || **pc == '\t' || **pc == '\n')++*pc;
  }

  return Value(array);
}

Value Value::parse_string(char **pc, char quote) {
  int32_t len;
  char *p = *pc;

  // calculate length
  do {
    while (*p && *++p != quote)
      ;
  } while (*p && *(p - 1) == '\\');

  if (*p != quote) {
    std::string msg = "missing closing ";
    msg.append(&quote);
    throw Exception::parser_error(msg);
  }
  len = p - *pc;
  std::string s;

  p = *pc;
  ++*pc;
  while (**pc != '\0' && (*pc - p < len)) {
    char *pc_i = *pc;
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

Value Value::parse_single_quoted_string(char **pc) {
  return parse_string(pc, '\'');
}

Value Value::parse_double_quoted_string(char **pc) {
  return parse_string(pc, '"');
}

Value Value::parse_number(char **pcc) {
  Value ret_val;
  char *pc = *pcc;

  // Sign can appear at the beggining
  if (*pc == '-' || *pc == '+')
    ++pc;

  // Continues while there are digits
  while (*pc && IS_DIGIT(*++pc));

  bool is_integer = true;
  if (tolower(*pc) == '.') {
    is_integer = false;

    // Skips the .
    ++pc;

    // Continues while there are digits
    while (*pc && IS_DIGIT(*++pc));
  }

  if (tolower(*pc) == 'e') // exponential
  {
    is_integer = false;

    // Skips the e
    ++pc;

    // Sign can appear for exponential numbers
    if (*pc == '-' || *pc == '+')
      ++pc;

    // Continues while there are digits
    while (*pc && IS_DIGIT(*++pc));
  }

  size_t len = pc - *pcc;
  std::string number(*pcc, len);

  if (is_integer) {
    int64_t ll = 0;
    try {
      ll = boost::lexical_cast<int64_t>(number.c_str());
    } catch (boost::bad_lexical_cast &e) {
      std::string s = "Error parsing int: ";
      s += e.what();
      throw Exception::parser_error(s);
    }
    ret_val = Value(static_cast<int64_t>(ll));
  } else {
    double d = 0;
    try {
      d = boost::lexical_cast<double>(number.c_str());
    } catch (boost::bad_lexical_cast &e) {
      std::string s = "Error parsing float: ";
      s += +e.what();
      throw Exception::parser_error(s);
    }

    ret_val = Value(d);
  }

  *pcc = pc;

  return ret_val;
}

bool my_strnicmp(const char *c1, const char *c2, size_t n) {
  return boost::iequals(boost::make_iterator_range(c1, c1 + n), boost::make_iterator_range(c2, c2 + n));
}

Value Value::parse(const std::string &s) {
  char *pc = const_cast<char *>(s.c_str());
  return parse(&pc);
}

Value Value::parse(char **pc) {
  if (**pc == '{') {
    return parse_map(pc);
  } else if (**pc == '[') {
    return parse_array(pc);
  } else if (**pc == '"') {
    return parse_double_quoted_string(pc);
  } else if (**pc == '\'') {
    return parse_single_quoted_string(pc);
  } else {
    if (IS_DIGIT(**pc) || **pc == '-' || **pc == '+') // a number
    {
      return parse_number(pc);
    } else // a constant between true, false, null
    {
      const char *pi = *pc;
      int n;
      while (*pc && IS_ALPHA(**pc))
        ++*pc;

      n = *pc - pi;
      if (n == 9 && ::my_strnicmp(pi, "undefined", 9)) {
        return Value();
      } else if (n == 4 && ::my_strnicmp(pi, "true", 4)) {
        return Value(true);
      } else if (n == 4 && ::my_strnicmp(pi, "null", 4)) {
        return Value::Null();
      } else if (n == 5 && ::my_strnicmp(pi, "false", 5)) {
        return Value(false);
      } else {
        throw Exception::parser_error(std::string("Can't parse '") + *pc + "'");
        //report_error(pi - _pc_json_start,
        //  "one of (array, string, number, true, false, null) expected");
        return Value();
      }
    }
  }

  return Value();
}

bool Value::operator == (const Value &other) const {
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
            return value.i == (int)other.value.d && ((other.value.d - (int)other.value.d) == 0.0);
          default:
            return false;
        }
      case UInteger:
        switch (other.type) {
          case Bool:
            return other.operator==(*this);
          case Float:
            return value.ui == (unsigned int)other.value.d && ((other.value.d - (int)other.value.d) == 0.0);
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
  append_descr(s, pprint ? 0 : -1, false); // top level strings are not quoted
  return s;
}

std::string Value::repr() const {
  std::string s;
  append_repr(s);
  return s;
}

std::string &Value::append_descr(std::string &s_out, int indent, int quote_strings) const {
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
    {
      boost::format fmt("%ld");
      fmt % value.i;
      s_out += fmt.str();
    }
    break;
    case UInteger:
    {
      boost::format fmt("%ld");
      fmt % value.ui;
      s_out += fmt.str();
    }
    break;
    case Float:
    {
      boost::format fmt("%g");
      fmt % value.d;
      s_out += fmt.str();
    }
    break;
    case String:
      if (quote_strings)
        s_out += (char)quote_strings + *value.s + (char)quote_strings;
      else
        s_out += *value.s;
      break;
    case Object:
      if (!value.o || !*value.o)
        throw Exception::value_error("Invalid object value encountered");
      as_object()->append_descr(s_out, indent, quote_strings);
      break;
    case Array:
    {
      if (!value.array || !*value.array)
        throw Exception::value_error("Invalid array value encountered");
      Array_type *vec = value.array->get();
      Array_type::iterator myend = vec->end(), mybegin = vec->begin();
      s_out += "[";
      for (Array_type::iterator iter = mybegin; iter != myend; ++iter) {
        if (iter != mybegin)
          s_out += ",";
        s_out += nl;
        if (indent >= 0)
          s_out.append((indent + 1) * 4, ' ');
        iter->append_descr(s_out, indent < 0 ? indent : indent + 1, '"');
      }
      s_out += nl;
      if (indent > 0)
        s_out.append(indent * 4, ' ');
      s_out += "]";
    }
    break;
    case Map:
    {
      if (!value.map || !*value.map)
        throw Exception::value_error("Invalid map value encountered");
      Map_type *map = value.map->get();
      Map_type::iterator myend = map->end(), mybegin = map->begin();
      s_out += "{" + nl;
      for (Map_type::iterator iter = mybegin; iter != myend; ++iter) {
        if (iter != mybegin)
          s_out += ", " + nl;
        if (indent >= 0)
          s_out.append((indent + 1) * 4, ' ');
        s_out += "\"";
        s_out += iter->first;
        s_out += "\": ";
        iter->second.append_descr(s_out, indent < 0 ? indent : indent + 1, '"');
      }
      s_out += nl;
      if (indent > 0)
        s_out.append(indent * 4, ' ');
      s_out += "}";
    }
    break;
    case MapRef:
      s_out.append("mapref");
      break;
    case Function:
      // TODO:
      //value.func->get()->append_descr(s_out, pprint);
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
    case Integer:
    {
      boost::format fmt("%ld");
      fmt % value.i;
      s_out += fmt.str();
    }
    break;
    case UInteger:
    {
      boost::format fmt("%ld");
      fmt % value.ui;
      s_out += fmt.str();
    }
    break;
    case Float:
    {
      boost::format fmt("%g");
      fmt % value.d;
      s_out += fmt.str();
    }
    break;
    case String:
    {
      std::string &s = *value.s;
      s_out += "\"";
      for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        switch (c) {
          case '\n':
            s_out += "\\n";
            break;
          case '\"':
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
          case '\0':
            s_out += "\\\0";
          default:
            s_out += c;
        }
      }
      s_out += "\"";
    }
    break;
    case Object:
      s_out = (*value.o)->append_repr(s_out);
      break;
    case Array:
    {
      Array_type *vec = value.array->get();
      Array_type::iterator myend = vec->end(), mybegin = vec->begin();
      s_out += "[";
      for (Array_type::iterator iter = mybegin; iter != myend; ++iter) {
        if (iter != mybegin)
          s_out += ", ";
        iter->append_repr(s_out);
      }
      s_out += "]";
    }
    break;
    case Map:
    {
      Map_type *map = value.map->get();
      Map_type::iterator myend = map->end(), mybegin = map->begin();
      s_out += "{";
      for (Map_type::iterator iter = mybegin; iter != myend; ++iter) {
        if (iter != mybegin)
          s_out += ", ";
        s_out += "\"";
        s_out += iter->first;
        s_out += "\": ";
        iter->second.append_repr(s_out);
      }
      s_out += "}";
    }
    break;
    case MapRef:
      break;
    case Function:
      // TODO:
      //value.func->get()->append_repr(s_out);
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

void Value::check_type(Value_type t) const {
  if (type != t)
    throw Exception::type_error("Invalid typecast");
}

int64_t Value::as_int() const {
  int64_t ret_val;

  if (type == Integer)
    ret_val = value.i;
  else if (type == UInteger && value.ui <= uint64_t(std::numeric_limits<int64_t>::max()))
    ret_val = (int64_t)value.ui;
  else
    throw Exception::type_error("Invalid typecast");

  return ret_val;
}

uint64_t Value::as_uint() const {
  uint64_t ret_val;

  if (type == UInteger)
    ret_val = value.ui;
  else if (type == Integer && value.i >= 0)
    ret_val = (uint64_t)value.i;
  else
    throw Exception::type_error("Invalid typecast");

  return ret_val;
}

double Value::as_double() const {
  double ret_val;
  if (type == Float)
    ret_val = value.d;
  else if (type == Integer)
    ret_val = (double)value.i;
  else if (type == UInteger)
    ret_val = (double)value.ui;
  else
    throw Exception::type_error("Invalid typecast");

  return ret_val;
}

//---

const std::string &Argument_list::string_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  switch (at(i).type) {
    case String:
      return *at(i).value.s;
    default:
      throw Exception::type_error((boost::format("Argument #%1% is expected to be a string") % (i + 1)).str());
  };
}

bool Argument_list::bool_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  switch (at(i).type) {
    case Bool:
      return at(i).value.b;
    case Integer:
      return at(i).value.i != 0;
    case UInteger:
      return at(i).value.ui != 0;
    case Float:
      return at(i).value.d != 0.0;
    default:
      throw Exception::type_error((boost::format("Argument #%1% is expected to be a bool") % (i + 1)).str());
  }
}

int64_t Argument_list::int_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type == Integer)
    return at(i).value.i;
  else if (at(i).type == UInteger && at(i).value.ui <= std::numeric_limits<int64_t>::max())
    return static_cast<int64_t>(at(i).value.ui);
  else if (at(i).type == Bool)
    return at(i).value.b ? 1 : 0;
  else
    throw Exception::type_error((boost::format("Argument #%1% is expected to be an int") % (i + 1)).str());
}

uint64_t Argument_list::uint_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type == UInteger)
    return at(i).value.ui;
  else if (at(i).type == Integer && at(i).value.i >= 0)
    return static_cast<uint64_t>(at(i).value.i);
  else if (at(i).type == Bool)
    return at(i).value.b ? 1 : 0;
  else
    throw Exception::type_error((boost::format("Argument #%1% is expected to be an unsigned int") % (i + 1)).str());
}

double Argument_list::double_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");

  if (at(i).type == Float)
    return at(i).value.d;
  else if (at(i).type == Integer)
    return static_cast<double>(at(i).value.i);
  else if (at(i).type == UInteger)
    return static_cast<double>(at(i).value.ui);
  else if (at(i).type == Bool)
    return at(i).value.b ? 1.0 : 0.0;
  else
    throw Exception::type_error((boost::format("Argument #%1% is expected to be a double") % (i + 1)).str());
}

std::shared_ptr<Object_bridge> Argument_list::object_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Object)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be an object") % (i + 1)).str());
  return *at(i).value.o;
}

std::shared_ptr<Value::Map_type> Argument_list::map_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Map)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be a map") % (i + 1)).str());
  return *at(i).value.map;
}

std::shared_ptr<Value::Array_type> Argument_list::array_at(unsigned int i) const {
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Array)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be an array") % (i + 1)).str());
  return *at(i).value.array;
}

void Argument_list::ensure_count(unsigned int c, const char *context) const {
  if (c != size())
    throw Exception::argument_error((boost::format("Invalid number of arguments in %1%, expected %2% but got %3%") % context % c % size()).str());
}

void Argument_list::ensure_count(unsigned int minc, unsigned int maxc, const char *context) const {
  if (size() < minc || size() > maxc)
    throw Exception::argument_error((boost::format("Invalid number of arguments in %1%, expected %2% to %3% but got %4%") % context % minc % maxc % size()).str());
}

void Argument_list::ensure_at_least(unsigned int minc, const char *context) const {
  if (size() < minc)
    throw Exception::argument_error((boost::format("Invalid number of arguments in %1%, expected at least %2% but got %3%") % context % minc % size()).str());
}

//--

Argument_map::Argument_map() {

}

Argument_map::Argument_map(const Value::Map_type &map)
  : _map(map) {
}

const std::string &Argument_map::string_at(const std::string &key) const {
  Value v(at(key));
  switch (v.type) {
    case String:
      return *v.value.s;
    default:
      throw Exception::type_error(std::string("Argument ").append(key).append(" is expected to be a string"));
  }
}

bool Argument_map::bool_at(const std::string &key) const {
  Value value(at(key));
  switch (value.type) {
    case Bool:
      return value.value.b;
    case Integer:
      return value.value.i != 0;
    case UInteger:
      return value.value.ui != 0;
    case Float:
      return value.value.d != 0.0;
    default:
      throw Exception::type_error(std::string("Argument '"+key+"' is expected to be a bool"));
  }
}

int64_t Argument_map::int_at(const std::string &key) const {
  Value value(at(key));
  if (value.type == Integer)
    return value.value.i;
  else if (value.type == UInteger && value.value.ui <= std::numeric_limits<int64_t>::max())
    return static_cast<int64_t>(value.value.ui);
  else if (value.type == Float)
    return static_cast<int64_t>(value.value.d);
  else if (value.type == Bool)
    return value.value.b ? 1 : 0;
  else
    throw Exception::type_error("Argument '"+key+"' is expected to be an int");
}

uint64_t Argument_map::uint_at(const std::string &key) const {
  Value value(at(key));
  if (value.type == UInteger)
    return value.value.ui;
  else if (value.type == Integer && value.value.i >= 0)
    return static_cast<uint64_t>(value.value.i);
  else if (value.type == Float)
    return static_cast<int64_t>(value.value.d);
  else if (value.type == Bool)
    return value.value.b ? 1 : 0;
  else
    throw Exception::type_error("Argument '"+key+"' is expected to be an unsigned int");
}

double Argument_map::double_at(const std::string &key) const {
  Value value(at(key));
  if (value.type == Float)
    return value.value.d;
  else if (value.type == Integer)
    return static_cast<double>(value.value.i);
  else if (value.type == UInteger)
    return static_cast<double>(value.value.ui);
  else if (value.type == Bool)
    return value.value.b ? 1.0 : 0.0;
  else
    throw Exception::type_error("Argument '"+key+"' is expected to be a double");
}

std::shared_ptr<Object_bridge> Argument_map::object_at(const std::string &key) const {
  Value value(at(key));
  if (value.type != Object)
    throw Exception::type_error("Argument '"+key+"' is expected to be an object");
  return *value.value.o;
}

std::shared_ptr<Value::Map_type> Argument_map::map_at(const std::string &key) const {
  Value value(at(key));
  if (value.type != Map)
    throw Exception::type_error("Argument '"+key+"' is expected to be a map");
  return *value.value.map;
}

std::shared_ptr<Value::Array_type> Argument_map::array_at(const std::string &key) const {
  Value value(at(key));
  if (value.type != Array)
    throw Exception::type_error("Argument '"+key+"' is expected to be an array");
  return *value.value.array;
}

void Argument_map::ensure_keys(const std::set<std::string> &mandatory_keys,
                               const std::set<std::string> &optional_keys,
                               const char *context) const {
  std::vector<std::string> invalid_keys;
  std::set<std::string> missing_keys(mandatory_keys);
  for (auto k : _map) {
    if (mandatory_keys.find(k.first) != mandatory_keys.end()) {
      missing_keys.erase(k.first);
    } else if (optional_keys.find(k.first) != optional_keys.end()) {
      // nop
    } else
      invalid_keys.push_back(k.first);
  }
  std::string msg;
  if (!invalid_keys.empty() && !missing_keys.empty()) {
    msg.append("Invalid and missing values in ").append(context).append(" ");
    msg.append("(invalid: ").append(join_strings(invalid_keys, ", "));
    msg.append("), (missing: ").append(join_strings(missing_keys, ", "));
    msg.append(")");
  } else if (!invalid_keys.empty()) {
    msg.append("Missing values in ").append(context).append(" ");
    msg.append(join_strings(invalid_keys, ", "));
  } else if (!missing_keys.empty()) {
    msg.append("Invalid values in ").append(context).append(" ");
    msg.append(join_strings(invalid_keys, ", "));
  }
  if (!msg.empty())
    throw Exception::argument_error(msg);
}


//--

void Object_bridge::append_json(JSON_dumper& dumper) const
{
  dumper.start_object();
  dumper.append_string("class", class_name());
  dumper.end_object();
}

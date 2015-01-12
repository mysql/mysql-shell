/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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
#include <stdexcept>
#include <cstdarg>
#include <boost/format.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdio>
#include <cstring>

using namespace shcore;

// --

Exception::Exception(const boost::shared_ptr<Value::Map_type> e)
: _error(e)
{
}


Exception Exception::argument_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ArgumentError");
  (*error)["message"] = Value(message);
  return Exception(error);
}


Exception Exception::attrib_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("AttributeError");
  (*error)["message"] = Value(message);
  return Exception(error);
}


Exception Exception::type_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("TypeError");
  (*error)["message"] = Value(message);
  return Exception(error);
}


Exception Exception::logic_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("LogicError");
  (*error)["message"] = Value(message);
  return Exception(error);
}


Exception Exception::scripting_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ScriptingError");
  (*error)["message"] = Value(message);
  return Exception(error);
}


Exception Exception::error_with_code(const std::string &type, const std::string &message, int code)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value(type);
  (*error)["message"] = Value(message);
  (*error)["code"] = Value(code);
  return Exception(error);
}

Exception Exception::error_with_code_and_state(const std::string &type, const std::string &message, int code, const char *sqlstate)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value(type);
  (*error)["message"] = Value(message);
  (*error)["code"] = Value(code);
  (*error)["state"] = Value(std::string(sqlstate));
  return Exception(error);
}

Exception Exception::parser_error(const std::string &message)
{
  boost::shared_ptr<Value::Map_type> error(new Value::Map_type());
  (*error)["type"] = Value("ParserError");
  (*error)["message"] = Value(message);
  return Exception(error);
}

const char *Exception::what() const BOOST_NOEXCEPT_OR_NOTHROW
{
  if ((*_error)["message"].type == String)
    return (*_error)["message"].value.s->c_str();
  return "?";
}

// --

std::string shcore::type_name(Value_type type)
{
  switch (type)
  {
    case Undefined:
      return "Undefined";
    case shcore::Null:
      return "Null";
    case Bool:
      return "Bool";
    case Integer:
      return "Integer";
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
  }
}

// --

std::string Value::Map_type::get_string(const std::string &k, const std::string &def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(String);
  return iter->second.as_string();
}

bool Value::Map_type::get_bool(const std::string &k, bool def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Bool);
  return iter->second.as_bool();
}

int64_t Value::Map_type::get_int(const std::string &k, int64_t def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Integer);
  return iter->second.as_int();
}

double Value::Map_type::get_double(const std::string &k, double def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Float);
  return iter->second.as_double();
}

boost::shared_ptr<Value::Map_type> Value::Map_type::get_map(const std::string &k,
                                                            boost::shared_ptr<Map_type> def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Map);
  return iter->second.as_map();
}

boost::shared_ptr<Value::Array_type> Value::Map_type::get_array(const std::string &k,
                                                                boost::shared_ptr<Array_type> def) const
{
  const_iterator iter = find(k);
  if (iter == end())
    return def;
  iter->second.check_type(Array);
  return iter->second.as_array();
}



Value::Value(const Value &copy)
: type(shcore::Null)
{
  operator=(copy);
}


Value::Value(const std::string &s)
: type(String)
{
  value.s = new std::string(s);
}


Value::Value(int i)
: type(Integer)
{
  value.i = i;
}


Value::Value(int64_t i)
: type(Integer)
{
  value.i = i;
}


Value::Value(double d)
: type(Float)
{
  value.d = d;
}


Value::Value(boost::shared_ptr<Function_base> f)
: type(Function)
{
  value.func = new boost::shared_ptr<Function_base>(f);
}


Value::Value(boost::shared_ptr<Object_bridge> n)
: type(Object)
{
  value.o = new boost::shared_ptr<Object_bridge>(n);
}

Value::Value(boost::shared_ptr<Map_type> n)
: type(Map)
{
  value.map = new boost::shared_ptr<Map_type>(n);
}

Value::Value(boost::weak_ptr<Map_type> n)
: type(MapRef)
{
  value.mapref = new boost::weak_ptr<Map_type>(n);
}

Value::Value(boost::shared_ptr<Array_type> n)
: type(Array)
{
  value.array = new boost::shared_ptr<Array_type>(n);
}


Value &Value::operator= (const Value &other)
{
  if (type == other.type)
  {
    switch (type)
    {
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
  }
  else
  {
    switch (type)
    {
      case Undefined:
      case shcore::Null:
      case Bool:
      case Integer:
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
    switch (type)
    {
      case Undefined:
      case shcore::Null:
        break;
      case Bool:
        value.b = other.value.b;
        break;
      case Integer:
        value.i = other.value.i;
        break;
      case Float:
        value.d = other.value.d;
        break;
      case String:
        value.s = new std::string(*other.value.s);
        break;
      case Object:
        value.o = new boost::shared_ptr<Object_bridge>(*other.value.o);
        break;
      case Array:
        value.array = new boost::shared_ptr<Array_type>(*other.value.array);
        break;
      case Map:
        value.map = new boost::shared_ptr<Map_type>(*other.value.map);
        break;
      case MapRef:
        value.mapref = new boost::weak_ptr<Map_type>(*other.value.mapref);
        break;
      case Function:
        value.func = new boost::shared_ptr<Function_base>(*other.value.func);
        break;
    }
  }
  return *this;
}


Value Value::parse_single_quoted_string(const char *pc)
{
  int32_t len;
  const char *p = pc;

  // calculate length
  do
  {
    while (*p && *++p != '\'')
      ;
  } while (*p && *(p - 1) == '\\');

  if (*p != '\'')
  {
    std::string msg = "missing closing \'";
    throw Exception::parser_error(msg);
  }
  len = p - pc;
  std::string s;

  p = pc;
  ++p;
  while (*p != '\0' && (p - pc < len))
  {
	const char *pc_i = p;
    if (*pc_i == '\\')
    {
	  switch( *(pc_i + 1))
	  {
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
      p = pc_i + 2;
    }
    else
    {
      s.append(p++, 1);
    }
  }

  return Value(s);
}


Value Value::parse_double_quoted_string(const char *pc)
{
  int32_t len;
  const char *p = pc;

  // calculate length
  do 
  {
    while (*p && *++p != '"')
      ;
  } while (*p && *(p - 1) == '\\');

  if (*p != '"')
  {
    throw Exception::parser_error("missing closing \"");
  }
  len = p - pc;
  std::string s;
  
  p = pc;
  ++p;
  while (*p != '\0' && (p - pc < len))
  {
	const char *pc_i = p;
    if (*pc_i == '\\')
    {
	  switch( *(pc_i + 1))
	  {
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
      p = pc_i + 2;
    }
    else 
    {
      s.append(p++, 1);
    }
  }

  return Value(s);
}


Value Value::parse_number(const char *pcc)
{
  const char *pc = pcc;
  bool hasSign = false;
  const char *pce = pc;

  if (*pc == '-')
  {
    hasSign = true;
    ++pc;
  }
  else if (*pc == '+')
  {
    ++pc;
  }
  while (*pc && isdigit(*++pc))
    ;
  if (tolower(*pc) == 'e' || *pc == '.') // floating point
  {
    double d = 0;
    try {
    	d= boost::lexical_cast<double>(pce);
    } catch( boost::bad_lexical_cast &e ) {
    	std::string s = "Error parsing float: ";
    	s += + e.what();
    	throw Exception::parser_error(s);
    }

    return Value(d);
  }
  else
  { // int point
    int64_t ll = 0;
	try {
		ll = boost::lexical_cast<int64_t>(pce);
	} catch( boost::bad_lexical_cast &e ) {
		std::string s = "Error parsing int: ";
		s += e.what();
		throw Exception::parser_error(s);
	}
    return Value(static_cast<int64_t>(ll));
  }

  return Value();
}

bool my_strnicmp(const char *c1, const char *c2, size_t n)
{
  return boost::iequals(boost::make_iterator_range(c1, c1+n), boost::make_iterator_range(c2, c2+n));
}

Value Value::parse(const std::string &s)
{
  const char *pc = s.c_str();
  
  if (*pc == '"')
  {
    return parse_double_quoted_string(pc);
  }
  else if (*pc == '\'')
  {
    return parse_single_quoted_string(pc);
  }
  else
  {
    if (isdigit(*pc) || *pc == '-' || *pc == '+') // a number
    {
      return parse_number(pc);
    }
    else // a constant between true, false, null
    {
      const char *pi = pc;
      int n;
      while (*pc && isalpha(*pc))
        ++pc;

      n = pc - pi;
      if (n == 9 && ::my_strnicmp(pi, "Undefined", 9))
      {
        return Value();
      }
      else if (n == 4 && ::strncmp(pi, "true", 4) == 0)
      {
        return Value(true);
      }
      else if (n == 5 && ::strncmp(pi, "false", 5) == 0)
      {
        return Value(false);
      }
      else
      {
        throw Exception::parser_error(std::string("Can't parse ")+pc);
        //report_error(pi - _pc_json_start,
        //  "one of (array, string, number, true, false, null) expected");
        return Value();
      }
    }
  }

  return Value();
}


bool Value::operator == (const Value &other) const
{
  if (type == other.type)
  {
    switch (type)
    {
      case Undefined:
        return true;  // undefined == undefined is true
      case shcore::Null:
        return true;
      case Bool:
        return value.b == other.value.b;
      case Integer:
        return value.i == other.value.i;
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
  }
  else
  {
    // with type conversion
    switch (type)
    {
      case Undefined:
        return false;
      case shcore::Null:
        return false;
      case Bool:
        switch (other.type)
        {
          case Integer:
            if (other.value.i == 1)
              return value.b == true;
            else if (other.value.i == 0)
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
        switch (other.type)
        {
          case Bool:
            return other.operator==(*this);
          case Float:
            return value.i == (int)other.value.d && ((other.value.d - (int)other.value.d) == 0.0);
          default:
            return false;
        }
      case Float:
        switch (other.type)
        {
          case Bool:
            return other.operator==(*this);
          case Integer:
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


std::string Value::descr(bool pprint) const
{
  std::string s;
  append_descr(s, pprint ? 0 : -1, false); // top level strings are not quoted
  return s;
}

std::string Value::repr() const
{
  std::string s;
  append_repr(s);
  return s;
}


std::string &Value::append_descr(std::string &s_out, int indent, int quote_strings) const
{
  std::string nl = (indent >= 0)? "\n" : "";
  switch (type)
  {
    case Undefined:
      s_out.append("Undefined");
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
      as_object()->append_descr(s_out, indent, quote_strings);
      break;
    case Array:
    {
      Array_type *vec = value.array->get();
      Array_type::iterator myend = vec->end(), mybegin = vec->begin();
      s_out += "[";
      for (Array_type::iterator iter = mybegin; iter != myend; ++iter)
      {
        if (iter != mybegin)
          s_out += ",";
        s_out += nl;
        if (indent >= 0)
          s_out.append((indent+1)*4, ' ');
        iter->append_descr(s_out, indent < 0 ? indent : indent+1, '"');
      }
      s_out += nl;
      if (indent > 0)
        s_out.append(indent*4, ' ');
      s_out += "]";
    }
      break;
    case Map:
    {
      Map_type *map = value.map->get();
      Map_type::iterator myend = map->end(), mybegin = map->begin();
      s_out += "{"+nl;
      for (Map_type::iterator iter = mybegin; iter != myend; ++iter)
      {
        if (iter != mybegin)
          s_out += ", " + nl;
        if (indent >= 0)
          s_out.append((indent+1)*4, ' ');
        s_out += "\"";
        s_out += iter->first;
        s_out += "\": ";
        iter->second.append_descr(s_out, indent < 0 ? indent : indent+1, '"');
      }
      s_out += nl;
      if (indent > 0)
        s_out.append(indent*4, ' ');
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


std::string &Value::append_repr(std::string &s_out) const
{
  switch (type)
  {
    case Undefined:
      s_out.append("Undefined");
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
      for (size_t i = 0; i < s.length(); i++)
      {
        char c = s[i];
        switch (c)
        {
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
      for (Array_type::iterator iter = mybegin; iter != myend; ++iter)
      {
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
      for (Map_type::iterator iter = mybegin; iter != myend; ++iter)
      {
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


Value::~Value()
{
  switch (type)
  {
    case Undefined:
    case shcore::Null:
    case Bool:
    case Integer:
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

void Value::check_type(Value_type t) const
{
  if (type != t)
    throw Exception::type_error("Invalid typecast");
}

//---

const std::string &Argument_list::string_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != String)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be a string") % (i+1)).str());
  return *at(i).value.s;
}


bool Argument_list::bool_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Bool)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be a bool") % (i+1)).str());
  return at(i).value.b;
}


int64_t Argument_list::int_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Integer)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be an int") % (i+1)).str());
  return at(i).value.i;
}


double Argument_list::double_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Float)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be a double") % (i+1)).str());
  return at(i).value.d;
}


boost::shared_ptr<Object_bridge> Argument_list::object_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Object)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be an object") % (i+1)).str());
  return *at(i).value.o;
}


boost::shared_ptr<Value::Map_type> Argument_list::map_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Map)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be a map") % (i+1)).str());
  return *at(i).value.map;
}


boost::shared_ptr<Value::Array_type> Argument_list::array_at(int i) const
{
  if (i >= size())
    throw Exception::argument_error("Insufficient number of arguments");
  if (at(i).type != Array)
    throw Exception::type_error((boost::format("Argument #%1% is expected to be an array") % (i+1)).str());
  return *at(i).value.array;
}


void Argument_list::ensure_count(int c, const char *context) const
{
  if (c != size())
    throw Exception::argument_error((boost::format("Invalid number of arguments in %1%, expected %2% but got %3%") % context % c % size()).str());
}


void Argument_list::ensure_count(int minc, int maxc, const char *context) const
{
  if (size() < minc || size() > maxc)
    throw Exception::argument_error((boost::format("Invalid number of arguments in %1%, expected %2% to %3% but got %4%") % context % minc % maxc % size()).str());
}

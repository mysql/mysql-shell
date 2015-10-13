/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "mod_mysqlx_constant.h"
#include "shellcore/object_factory.h"
#include <boost/algorithm/string.hpp>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

using namespace shcore;
using namespace mysh::mysqlx;

std::map<std::string, boost::shared_ptr<Constant> > Constant::_constants;

Constant::Constant(const std::string &group, const std::string &id, const std::vector<shcore::Value> *params) :
_group(group), _id(id)
{
  _data = get_constant_value(_group, _id, params);
}

std::vector<std::string> Constant::get_members() const
{
  std::vector<std::string> members;
  members.push_back("data");
  return members;
}

Value Constant::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  if (prop == "data")
    ret_val = _data;
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

bool Constant::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}

// Retrieves the internal constant value based on a Group and ID
// An exception is produced if invalid group.id data is provided
// But this should not happen as it is used internally only
Value Constant::get_constant_value(const std::string& group, const std::string& id, const std::vector<shcore::Value> *params)
{
  Value ret_val;

  // By default all is OK if there are NO params
  // Only varchar, char and decimal will allow params
  bool valid_params = true;
  size_t param_count = 0;

  if (params)
  {
    param_count = params->size();
    valid_params = (param_count == 0);
  }

  if (group == "DataTypes")
  {
    if (id == "TinyInt"){ ret_val = Value("TINYINT"); }
    else if (id == "SmallInt"){ ret_val = Value("SMALLINT"); }
    else if (id == "MediumInt"){ ret_val = Value("MEDIUMINT"); }
    else if (id == "Int"){ ret_val = Value("INT"); }
    else if (id == "Integer"){ ret_val = Value("INTEGER"); }
    else if (id == "BigInt"){ ret_val = Value("BIGINT"); }
    else if (id == "Real"){ ret_val = Value("REAL"); }
    else if (id == "Float"){ ret_val = Value("FLOAT"); }
    else if (id == "Double"){ ret_val = Value("DOUBLE"); }
    else if (id == "Decimal" || id == "Numeric")
    {
      std::string data = id == "Decimal" ? "DECIMAL" : "NUMERIC";

      bool error = false;
      if (param_count)
      {
        if (param_count <= 2)
        {
          if (params->at(0).type == shcore::Integer)
            data += "(" + params->at(0).descr(false);
          else
            error = true;

          if (param_count == 2)
          {
            if (params->at(1).type == shcore::Integer)
              data += "," + params->at(1).descr(false);
            else
              error = true;
          }
        }
        else
          error = true;

        if (error)
          throw shcore::Exception::argument_error("DataTypes.Decimal allows up to two numeric parameters precision and scale");

        data += ")";
      }
      ret_val = Value(data);
    }
    else if (id == "Date"){ ret_val = Value("DATE"); }
    else if (id == "Time"){ ret_val = Value("TIME"); }
    else if (id == "Timestamp"){ ret_val = Value("TIMESTAMP"); }
    else if (id == "DateTime"){ ret_val = Value("DATETIME"); }
    else if (id == "Year"){ ret_val = Value("YEAR"); }
    else if (id == "Varchar" || id == "Char")
    {
      std::string data = id == "Varchar" ? "VARCHAR" : "CHAR";

      if (param_count)
      {
        if (param_count == 1 && params->at(0).type == shcore::Integer)
          data += "(" + params->at(0).descr(false) + ")";
        else
          throw shcore::Exception::argument_error("DataTypes." + id + " only allows a numeric parameter for length");
      }
      ret_val = Value(data);
    }
    else if (id == "Bit"){ ret_val = Value("BIT"); }
    else if (id == "Blob"){ ret_val = Value("BLOB"); }
    else if (id == "Text"){ ret_val = Value("TEXT"); }
  }
  else if (group == "IndexTypes")
  if (id == "IndexUnique"){ ret_val = Value::True(); }
  else
    throw shcore::Exception::logic_error("Invalid group onconstant definition:" + group + "." + id);

  if (!ret_val)
    throw shcore::Exception::logic_error("Invalid id on constant definition:" + group + "." + id);

  return ret_val;
}

boost::shared_ptr<shcore::Object_bridge> Constant::create(const shcore::Argument_list &args)
{
  args.ensure_count(2, 4, "mysqlx.Constant");

  boost::shared_ptr<shcore::Object_bridge> ret_val;

  std::string group;
  std::string param_id;

  try
  {
    group = args.string_at(0);

    std::vector<shcore::Value> params;
    for (size_t index = 2; index < args.size(); index++)
    {
      param_id += "_" + args[index].descr(false);
      params.push_back(args[index]);
    }

    ret_val = create(args.string_at(0), args.string_at(1), param_id, &params);
  }
  catch (shcore::Exception &e)
  {
    std::string error = e.what();
    throw shcore::Exception::argument_error("mysqlx.Constant: " + error);
  }

  return ret_val;
}

boost::shared_ptr<shcore::Object_bridge> Constant::create(const std::string& group, const std::string &id, const std::string &param_id, const std::vector<shcore::Value> *params)
{
  std::string key = group + "." + id + param_id;

  if (_constants.find(key) == _constants.end())
  {
    boost::shared_ptr<Constant> constant(new Constant(group, id, params));

    _constants[key] = constant;
  }

  return _constants[key];
}

std::string &Constant::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const
{
  s_out.append("<" + _group + "." + _id);

  if (_data.type == shcore::String)
  {
    std::string data = _data.as_string();
    size_t pos = data.find("(");
    if (pos != std::string::npos)
      s_out.append(data.substr(pos));
  }

  s_out.append(">");

  return s_out;
}

void Constant::set_param(const std::string& data)
{
  std::string temp = _data.as_string();
  temp += "(" + data + ")";
  _data = Value(temp);
}
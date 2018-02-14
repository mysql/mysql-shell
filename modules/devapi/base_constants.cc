/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/devapi/base_constants.h"
#include <map>
#include <memory>
#include <string>
#include "scripting/object_factory.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

using namespace shcore;
using namespace mysqlsh;

std::map<std::string, Constant::Module_constants> Constant::_constants;

Constant::Constant(const std::string &module, const std::string &group,
                   const std::string &id, const shcore::Argument_list &args)
    : _module(module), _group(group), _id(id) {
  _data = get_constant_value(_module, _group, _id, args);

  add_property("data");
}

Value Constant::get_member(const std::string &prop) const {
  // Retrieves the member first from the parent
  Value ret_val;

  if (prop == "data")
    ret_val = _data;
  else
    ret_val = Cpp_object_bridge::get_member(prop);

  return ret_val;
}

bool Constant::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name() && this == &other;
}

Value Constant::get_constant(const std::string &module,
                             const std::string &group, const std::string &id,
                             const shcore::Argument_list &args) {
  Value ret_val;

  if (_constants.find(module) != _constants.end()) {
    if (_constants.at(module).find(group) != _constants.at(module).end()) {
      if (_constants.at(module).at(group).find(id) !=
          _constants.at(module).at(group).end())
        ret_val = shcore::Value(std::static_pointer_cast<shcore::Object_bridge>(
            _constants.at(module).at(group).at(id)));
    }
  }

  if (!ret_val) {
    std::shared_ptr<Constant> constant(new Constant(module, group, id, args));

    if (constant->data()) {
      if (_constants.find(module) == _constants.end()) {
        Module_constants new_module;
        _constants.insert({module, new_module});
      }

      if (_constants.at(module).find(group) == _constants.at(module).end()) {
        Group_constants new_group;
        _constants.at(module).insert({group, new_group});
      }

      if (_constants.at(module).at(group).find(id) ==
          _constants.at(module).at(group).end())
        _constants.at(module).at(group).insert({id, constant});

      ret_val = shcore::Value(
          std::static_pointer_cast<shcore::Object_bridge>(constant));
    }
  }

  return ret_val;
}

// Retrieves the internal constant value based on a Group and ID
// An exception is produced if invalid group.id data is provided
// But this should not happen as it is used internally only
Value Constant::get_constant_value(const std::string &module,
                                   const std::string &group,
                                   const std::string &id,
                                   const shcore::Argument_list &/*args*/) {
  Value ret_val;

  // By default all is OK if there are NO params
  // Only varchar, char and decimal will allow params

  // size_t param_count = 0;

  // param_count = args.size();

  if (module == "mysqlx" || module == "mysql") {
    if (group == "Type") {
      if (id == "BIT") {
        ret_val = Value("BIT");
      } else if (id == "TINYINT") {
        ret_val = Value("TINYINT");
      } else if (id == "SMALLINT") {
        ret_val = Value("SMALLINT");
      } else if (id == "MEDIUMINT") {
        ret_val = Value("MEDIUMINT");
      } else if (id == "INT") {
        ret_val = Value("INT");
      } else if (id == "BIGINT") {
        ret_val = Value("BIGINT");
      } else if (id == "FLOAT") {
        ret_val = Value("FLOAT");
      } else if (id == "DECIMAL") {
        ret_val = Value("DECIMAL");
      } else if (id == "DOUBLE") {
        ret_val = Value("DOUBLE");
      }
      // Commenting this, could be useful when we change all the constats to
      // function calls
      // On MySQL 8 S II
      //       else if (id == "Decimal" || id == "Numeric")
      //       {
      // 	std::string data = id == "Decimal" ? "DECIMAL" : "NUMERIC";
      //
      // 	bool error = false;
      // 	if (param_count)
      // 	{
      // 	  if (param_count <= 2)
      // 	  {
      // 	    if (args.at(0).type == shcore::Integer)
      // 	      data += "(" + args.at(0).descr(false);
      // 	    else
      // 	      error = true;
      //
      // 	    if (param_count == 2)
      // 	    {
      // 	      if (args.at(1).type == shcore::Integer)
      // 		data += "," + args.at(1).descr(false);
      // 	      else
      // 		error = true;
      // 	    }
      // 	  }
      // 	  else
      // 	    error = true;
      //
      // 	  if (error)
      // 	    throw shcore::Exception::argument_error("Type.Decimal allows
      // up to two numeric parameters precision and scale");
      //
      // 	  data += ")";
      // 	}
      // 	ret_val = Value(data);
      //       }

      // These are new
      else if (id == "JSON") {
        ret_val = Value("JSON");
      } else if (id == "STRING") {
        ret_val = Value("STRING");
      } else if (id == "BYTES") {
        ret_val = Value("BYTES");
      } else if (id == "TIME") {
        ret_val = Value("TIME");
      } else if (id == "DATE") {
        ret_val = Value("DATE");
      } else if (id == "DATETIME") {
        ret_val = Value("DATETIME");
      } else if (id == "SET") {
        ret_val = Value("SET");
      } else if (id == "ENUM") {
        ret_val = Value("ENUM");
      } else if (id == "GEOMETRY") {
        ret_val = Value("GEOMETRY");
      // NULL is only registered for mysql module
      } else if (id == "NULL" && module == "mysql") {
        ret_val = Value("NULL");
      }
    } else if (group == "LockContention") {
      if (id == "NOWAIT") {
        ret_val = Value("NOWAIT");
      } else if (id == "SKIP_LOCK") {
        ret_val = Value("SKIP_LOCK");
      } else if (id == "DEFAULT") {
        ret_val = Value("DEFAULT");
      }
    } else {
      throw shcore::Exception::logic_error(
          "Invalid group on constant definition:" + group + "." + id);
    }
  } else {
    throw shcore::Exception::logic_error(
        "Invalid module on constant definition:" + group + "." + id);
  }

  return ret_val;
}

std::string &Constant::append_descr(std::string &s_out, int UNUSED(indent),
                                    int UNUSED(quote_strings)) const {
  s_out.append("<" + _group + "." + _id);

  if (_data.type == shcore::String) {
    std::string data = _data.as_string();
    size_t pos = data.find("(");
    if (pos != std::string::npos)
      s_out.append(data.substr(pos));
  }

  s_out.append(">");

  return s_out;
}

void Constant::set_param(const std::string &data) {
  std::string temp = _data.as_string();
  temp += "(" + data + ")";
  _data = Value(temp);
}

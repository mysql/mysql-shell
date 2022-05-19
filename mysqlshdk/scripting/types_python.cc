/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
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

#include "scripting/types_python.h"
#include "scripting/common.h"
#include "scripting/object_factory.h"

using namespace shcore;

Python_function::Python_function(Python_context *context, PyObject *function)
    : _py(context) {
  m_function = _py->store(function);

  py::Release name{PyObject_GetAttrString(function, "__name__")};
  Python_context::pystring_to_string(name.get(), &m_name);

  // Gets the number of arguments on the python function definition
  // NOTE: **kwargs is not accounted on co_argcount
  // This will be used to determine when the function should be called using
  // kwargs or not
  auto fcode = PyFunction_GetCode(function);
  py::Release arg_count{PyObject_GetAttrString(fcode, "co_argcount")};
  auto varg_count = _py->convert(arg_count.get());
  m_arg_count = varg_count.as_uint();
}

Python_function::~Python_function() {
  WillEnterPython lock;
  if (auto ref = m_function.lock()) {
    _py->erase(ref);
  }
}

const std::vector<std::pair<std::string, Value_type>>
    &Python_function::signature() const {
  // TODO:
  static std::vector<std::pair<std::string, Value_type>> tmp;
  return tmp;
}

Value_type Python_function::return_type() const {
  // TODO:
  return Undefined;
}

bool Python_function::operator==(const Function_base &UNUSED(other)) const {
  // TODO:
  return false;
}

bool Python_function::operator!=(const Function_base &UNUSED(other)) const {
  // TODO:
  return false;
}

Value Python_function::invoke(const Argument_list &args) {
  if (auto function = m_function.lock()) {
    WillEnterPython lock;

    auto argc = args.size();

    // If the function caller provides more parameters than the ones defined in
    // the function, the last parameter should be handled as follows:
    // - If Dictionary, it's data is passed as kwargs
    // - If Undefined, then kwards is empty
    // - Any other case will fall into passing it as normal parameter
    py::Release kw_args;

    if (argc == (m_arg_count + 1) &&
        (args[argc - 1].type == shcore::Value_type::Map ||
         args[argc - 1].type == shcore::Value_type::Undefined)) {
      // We remove the last parameter from the parameter list
      argc--;

      // Sets the kwargs from the dictionary if any
      if (args[argc].type == shcore::Value_type::Map) {
        kw_args = py::Release{PyDict_New()};
        auto kwd_dictionary = args[argc].as_map();
        for (auto item = kwd_dictionary->begin(); item != kwd_dictionary->end();
             item++) {
          auto conv = _py->convert(item->second);
          PyDict_SetItemString(kw_args.get(), item->first.c_str(), conv.get());
        }
      }
    }

    py::Release ret_val;
    {
      py::Release argv{PyTuple_New(argc)};

      for (size_t index = 0; index < argc; ++index)
        PyTuple_SetItem(argv.get(), index, _py->convert(args[index]).release());

      ret_val = py::Release{
          PyObject_Call(function->get(), argv.get(), kw_args.get())};
    }

    if (!ret_val) {
      // converts mysqlsh.Error to shcore::Error and throws the exception, if
      // the Python exception is something else, then just returns
      _py->throw_if_mysqlsh_error();

      constexpr auto error = "User-defined function threw an exception";
      std::string details = _py->fetch_and_clear_exception();

      if (!details.empty()) {
        details = ": " + details;
      }

      throw Exception::scripting_error(error + details);
    } else {
      return _py->convert(ret_val.get());
    }
  } else {
    throw Exception::scripting_error(
        "Bound function does not exist anymore, it seems that Python context "
        "was released");
  }
}

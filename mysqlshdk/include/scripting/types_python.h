/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates.
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

#ifndef _TYPES_PYTHON_H_
#define _TYPES_PYTHON_H_

// python_context.h has to be included first

#include "scripting/python_context.h"

#include <memory>

#include "scripting/types.h"

namespace shcore {

class SHCORE_PUBLIC Python_function : public Function_base {
 public:
  Python_function(Python_context *context, PyObject *function);
  ~Python_function() override;

  const std::string &name() const override { return m_name; }

  const std::vector<std::pair<std::string, Value_type>> &signature()
      const override;

  Value_type return_type() const override;

  bool operator==(const Function_base &other) const override;

  bool operator!=(const Function_base &other) const;

  Value invoke(const Argument_list &args) override;

 private:
  Python_context *_py;
  std::weak_ptr<py::Store> m_function;
  std::string m_name;
  uint64_t m_arg_count;
};

}  // namespace shcore

#endif

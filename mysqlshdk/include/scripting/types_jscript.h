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

#ifndef _TYPES_JSCRIPT_H_
#define _TYPES_JSCRIPT_H_

#include "scripting/jscript_context.h"

#include "scripting/types.h"

#include "scripting/include_v8.h"

namespace shcore {
class JScript_function : public Function_base {
public:
  JScript_function(JScript_context* context, v8::Handle<v8::Function> function);
  virtual ~JScript_function() {}

  const std::string &name() const override;

  const std::vector<std::pair<std::string, Value_type> > &signature() const override;

  Value_type return_type() const override;

  bool operator == (const Function_base &other) const override;

  bool operator != (const Function_base &other) const;

  Value invoke(const Argument_list &args) override;

  bool has_var_args() override { return false; }

private:
  JScript_context *_js;
  v8::Persistent<v8::Function> _function;
};
}  // namespace shcore

#endif

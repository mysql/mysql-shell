/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _JSCRIPT_CONTEXT_H_
#define _JSCRIPT_CONTEXT_H_

#include <string>
#include "scripting/include_v8.h"
#include "scripting/lang_base.h"
#include "scripting/types.h"

namespace shcore {
struct Interpreter_delegate;
class Object_registry;

class SHCORE_PUBLIC JScript_context {
public:
  JScript_context(Object_registry *registry, Interpreter_delegate *deleg);
  ~JScript_context();

  Value execute(const std::string &code, const std::string& source = "",
                const std::vector<std::string> &argv = {}) throw (Exception);
  Value execute_interactive(const std::string &code,
                            Input_state &r_state) noexcept;

  v8::Isolate *isolate() const;
  v8::Handle<v8::Context> context() const;

  Value v8_value_to_shcore_value(const v8::Handle<v8::Value> &value);
  v8::Handle<v8::Value> shcore_value_to_v8_value(const Value &value);
  Argument_list convert_args(const v8::FunctionCallbackInfo<v8::Value>& args);

  void set_global(const std::string &name, const Value &value);
  Value get_global(const std::string &name);

  void set_global_item(const std::string& global_name, const std::string& item_name, const Value &value);

  bool is_terminating() const { return _terminating; }
  void terminate();

private:
  struct JScript_context_impl;
  JScript_context_impl *_impl;
  bool _terminating = false;

  Object_registry *_registry;

  Value get_v8_exception_data(v8::TryCatch *exc, bool interactive);
  std::string format_exception(const shcore::Value &exc);
};
};

#endif

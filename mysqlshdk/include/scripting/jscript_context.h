/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _JSCRIPT_CONTEXT_H_
#define _JSCRIPT_CONTEXT_H_

#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "scripting/include_v8.h"
#include "scripting/lang_base.h"
#include "scripting/types.h"

namespace shcore {
struct Interpreter_delegate;
class Object_registry;

using JSObject =
    v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>>;

class SHCORE_PUBLIC JScript_context {
 public:
  JScript_context(Object_registry *registry);
  ~JScript_context();

  std::pair<Value, bool> execute(const std::string &code,
                                 const std::string &source = "",
                                 const std::vector<std::string> &argv = {});
  std::pair<Value, bool> execute_interactive(const std::string &code,
                                             Input_state *r_state) noexcept;

  v8::Isolate *isolate() const;
  v8::Local<v8::Context> context() const;

  Value v8_value_to_shcore_value(const v8::Local<v8::Value> &value);
  v8::Local<v8::Value> shcore_value_to_v8_value(const Value &value);
  Argument_list convert_args(const v8::FunctionCallbackInfo<v8::Value> &args);

  void set_global(const std::string &name, const Value &value);
  Value get_global(const std::string &name);

  std::tuple<JSObject, std::string> get_global_js(const std::string &name);

  std::tuple<bool, JSObject, std::string> get_member_of(
      const JSObject *obj, const std::string &name);

  std::vector<std::pair<bool, std::string>> get_members_of(const JSObject *obj);
  std::vector<std::pair<bool, std::string>> list_globals();

  void set_global_item(const std::string &global_name,
                       const std::string &item_name, const Value &value);

  bool is_terminating() const;
  void terminate();

  v8::Local<v8::String> v8_string(const char *data);

  v8::Local<v8::String> v8_string(const std::string &data);

  std::string to_string(v8::Local<v8::Value> obj);

 private:
  struct JScript_context_impl;
  JScript_context_impl *_impl;

  Value get_v8_exception_data(v8::TryCatch *exc, bool interactive);
  std::string format_exception(const shcore::Value &exc);
};

v8::Local<v8::String> v8_string(v8::Isolate *isolate, const char *data);

v8::Local<v8::String> v8_string(v8::Isolate *isolate, const std::string &data);

std::string to_string(v8::Isolate *isolate, v8::Local<v8::Value> obj);

};  // namespace shcore

#endif

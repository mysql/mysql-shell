/*
 * Copyright (c) 2014, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _JSCRIPT_MAP_WRAPPER_H_
#define _JSCRIPT_MAP_WRAPPER_H_

#include <memory>

#include "scripting/include_v8.h"
#include "scripting/types.h"

namespace shcore {
class JScript_context;

class JScript_map_wrapper {
 public:
  JScript_map_wrapper(JScript_context *context);
  ~JScript_map_wrapper();

  v8::Local<v8::Object> wrap(std::shared_ptr<Value::Map_type> map);

  static bool unwrap(v8::Local<v8::Object> value,
                     std::shared_ptr<Value::Map_type> *ret_map);

  static bool is_map(v8::Local<v8::Object> value);

 private:
  static void handler_getter(v8::Local<v8::Name> property,
                             const v8::PropertyCallbackInfo<v8::Value> &info);
  static void handler_deleter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Boolean> &info);
  static void handler_setter(v8::Local<v8::Name> property,
                             v8::Local<v8::Value> value,
                             const v8::PropertyCallbackInfo<v8::Value> &info);
  static void handler_enumerator(
      const v8::PropertyCallbackInfo<v8::Array> &info);

 private:
  JScript_context *_context;
  v8::Persistent<v8::ObjectTemplate> _map_template;
};
}  // namespace shcore

#endif

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

#ifndef _JSCRIPT_TYPE_CONVERSION_H_
#define _JSCRIPT_TYPE_CONVERSION_H_

#include "scripting/include_v8.h"
#include "scripting/types.h"

namespace shcore {
class JScript_context;

struct JScript_type_bridger {
  JScript_type_bridger(JScript_context *context);
  ~JScript_type_bridger();

  void init();
  void dispose();

  Value v8_value_to_shcore_value(const v8::Local<v8::Value> &value);
  v8::Local<v8::Value> shcore_value_to_v8_value(const Value &value);

  v8::Local<v8::String> type_info(v8::Local<v8::Value> value);

  double call_num_method(v8::Local<v8::Object> object, const char *method);

  v8::Local<v8::Value> native_object_to_js(Object_bridge_ref object);
  Object_bridge_ref js_object_to_native(v8::Local<v8::Object> object);

  JScript_context *owner;

  class JScript_object_wrapper *object_wrapper;
  class JScript_object_wrapper *indexed_object_wrapper;
  class JScript_function_wrapper *function_wrapper;
  class JScript_map_wrapper *map_wrapper;
  class JScript_array_wrapper *array_wrapper;
};
};  // namespace shcore

#endif

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

#ifndef _JSCRIPT_TYPE_CONVERSION_H_
#define _JSCRIPT_TYPE_CONVERSION_H_

#include "shellcore/types.h"
#include "shellcore/include_v8.h"

namespace shcore {
class JScript_context;

struct JScript_type_bridger {
  JScript_type_bridger(JScript_context *context);
  ~JScript_type_bridger();

  void init();
  void dispose();

  Value v8_value_to_shcore_value(const v8::Handle<v8::Value> &value);
  v8::Handle<v8::Value> shcore_value_to_v8_value(const Value &value);

  v8::Handle<v8::String> type_info(v8::Handle<v8::Value> value);

  double call_num_method(v8::Handle<v8::Object> object, const char *method);

  v8::Handle<v8::Value> native_object_to_js(Object_bridge_ref object);
  Object_bridge_ref js_object_to_native(v8::Handle<v8::Object> object);

  JScript_context *owner;

  class JScript_object_wrapper *object_wrapper;
  class JScript_object_wrapper *indexed_object_wrapper;
  class JScript_function_wrapper *function_wrapper;
  class JScript_map_wrapper *map_wrapper;
  class JScript_array_wrapper *array_wrapper;
};
};

#endif

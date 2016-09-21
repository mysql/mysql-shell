/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _JSCRIPT_MAP_WRAPPER_H_
#define _JSCRIPT_MAP_WRAPPER_H_

#include "shellcore/types.h"
#include "shellcore/include_v8.h"

namespace shcore {
class JScript_context;

class JScript_map_wrapper {
public:
  JScript_map_wrapper(JScript_context *context);
  ~JScript_map_wrapper();

  v8::Handle<v8::Object> wrap(std::shared_ptr<Value::Map_type> map);

  static bool unwrap(v8::Handle<v8::Object> value, std::shared_ptr<Value::Map_type> &ret_map);

private:
  struct Collectable;

  static void handler_getter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);
  static void handler_setter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info);
  static void handler_enumerator(const v8::PropertyCallbackInfo<v8::Array>& info);

  static void wrapper_deleted(const v8::WeakCallbackData<v8::Object, Collectable>& data);

private:
  JScript_context *_context;
  v8::Persistent<v8::ObjectTemplate> _map_template;
};
};

#endif

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

#include "scripting/jscript_map_wrapper.h"

#include "mysqlshdk/include/scripting/jscript_collectable.h"
#include "scripting/jscript_context.h"

#include <cstring>
#include <iostream>

namespace shcore {

namespace {

static int magic_pointer = 0;

using Map_collectable = Collectable<Value::Map_type>;

}  // namespace

JScript_map_wrapper::JScript_map_wrapper(JScript_context *context)
    : _context(context) {
  v8::Handle<v8::ObjectTemplate> templ =
      v8::ObjectTemplate::New(_context->isolate());
  _map_template.Reset(_context->isolate(), templ);

  // v8::NamedPropertyHandlerConfiguration config;
  // config.getter = &JScript_map_wrapper::handler_getter;
  // config.setter = &JScript_map_wrapper::handler_setter;
  // config.enumerator = &JScript_map_wrapper::handler_enumerator;
  // templ->SetHandler(config);
  templ->SetNamedPropertyHandler(&JScript_map_wrapper::handler_getter,
                                 &JScript_map_wrapper::handler_setter, 0, 0,
                                 &JScript_map_wrapper::handler_enumerator);

  templ->SetInternalFieldCount(3);
}

JScript_map_wrapper::~JScript_map_wrapper() { _map_template.Reset(); }

v8::Handle<v8::Object> JScript_map_wrapper::wrap(
    std::shared_ptr<Value::Map_type> map) {
  v8::Local<v8::ObjectTemplate> templ =
      v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _map_template);
  if (!templ.IsEmpty()) {
    v8::Local<v8::Object> self(templ->NewInstance());
    if (!self.IsEmpty()) {
      const auto holder = new Map_collectable(map, _context->isolate(), self);

      self->SetAlignedPointerInInternalField(0, &magic_pointer);
      self->SetAlignedPointerInInternalField(1, holder);
      self->SetAlignedPointerInInternalField(2, this);
    }
    return self;
  }
  return {};
}

void JScript_map_wrapper::handler_getter(
    v8::Local<v8::String> property,
    const v8::PropertyCallbackInfo<v8::Value> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  JScript_map_wrapper *self = static_cast<JScript_map_wrapper *>(
      obj->GetAlignedPointerFromInternalField(2));
  const auto &map =
      static_cast<Map_collectable *>(obj->GetAlignedPointerFromInternalField(1))
          ->data();
  if (!map) {
    info.GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        info.GetIsolate(), "Reference to invalid object"));
    return;
  }

  v8::String::Utf8Value prop(property);

  /*if (strcmp(prop, "__members__") == 0)
  {
  v8::Handle<v8::Array> marray = v8::Array::New(info.GetIsolate());
  int i = 0;
  for (Value::Map_type::const_iterator iter = (*map)->begin(); iter !=
  (*map)->end(); ++iter)
  {
  marray->Set(i++, v8::String::NewFromUtf8(info.GetIsolate(),
  iter->first.c_str()));
  }
  info.GetReturnValue().Set(marray);
  }
  else*/
  {
    Value::Map_type::const_iterator iter = map->find(*prop);
    if (iter != map->end())
      info.GetReturnValue().Set(
          self->_context->shcore_value_to_v8_value(iter->second));
  }
}

void JScript_map_wrapper::handler_setter(
    v8::Local<v8::String> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<v8::Value> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  JScript_map_wrapper *self = static_cast<JScript_map_wrapper *>(
      obj->GetAlignedPointerFromInternalField(2));
  const auto &map =
      static_cast<Map_collectable *>(obj->GetAlignedPointerFromInternalField(1))
          ->data();
  if (!map) {
    info.GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        info.GetIsolate(), "Reference to invalid object"));
    return;
  }

  v8::String::Utf8Value prop(property);
  (*map)[*prop] = self->_context->v8_value_to_shcore_value(value);

  info.GetReturnValue().Set(value);
}

void JScript_map_wrapper::handler_enumerator(
    const v8::PropertyCallbackInfo<v8::Array> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  const auto &map =
      static_cast<Map_collectable *>(obj->GetAlignedPointerFromInternalField(1))
          ->data();

  if (!map) throw std::logic_error("bug!");

  v8::Handle<v8::Array> marray = v8::Array::New(info.GetIsolate());
  int i = 0;
  for (const auto &iter : *map) {
    marray->Set(i++,
                v8::String::NewFromUtf8(info.GetIsolate(), iter.first.c_str()));
  }
  info.GetReturnValue().Set(marray);
}

bool JScript_map_wrapper::is_map(v8::Handle<v8::Object> value) {
  if (value->InternalFieldCount() == 3 &&
      value->GetAlignedPointerFromInternalField(0) == (void *)&magic_pointer) {
    return true;
  }
  return false;
}

bool JScript_map_wrapper::unwrap(v8::Handle<v8::Object> value,
                                 std::shared_ptr<Value::Map_type> &ret_object) {
  if (value->InternalFieldCount() == 3 &&
      value->GetAlignedPointerFromInternalField(0) == (void *)&magic_pointer) {
    const auto &object = static_cast<Map_collectable *>(
                             value->GetAlignedPointerFromInternalField(1))
                             ->data();
    ret_object = object;
    return true;
  }
  return false;
}

}  // namespace shcore

/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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
#include "mysqlshdk/include/scripting/jscript_iterator.h"
#include "scripting/jscript_context.h"

#include <cstring>
#include <iostream>

namespace shcore {

namespace {

static int magic_pointer = 0;

struct Map_config;

using Map_collectable = Collectable<Value::Map_type, Map_config>;

struct Map_config {
 public:
  static size_t get_allocated_size(const Map_collectable &c) {
    return 1024 * c.data()->size();
  }
};

}  // namespace

JScript_map_wrapper::JScript_map_wrapper(JScript_context *context)
    : _context(context) {
  const auto isolate = _context->isolate();
  v8::Local<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(isolate);
  _map_template.Reset(isolate, templ);

  v8::NamedPropertyHandlerConfiguration config;
  config.getter = &JScript_map_wrapper::handler_getter;
  config.setter = &JScript_map_wrapper::handler_setter;
  config.deleter = &JScript_map_wrapper::handler_deleter;
  config.enumerator = &JScript_map_wrapper::handler_enumerator;
  config.flags = v8::PropertyHandlerFlags::kOnlyInterceptStrings;
  templ->SetHandler(config);

  add_iterator<Map_collectable>(templ, isolate);
  templ->SetInternalFieldCount(2);
}

JScript_map_wrapper::~JScript_map_wrapper() { _map_template.Reset(); }

v8::Local<v8::Object> JScript_map_wrapper::wrap(
    std::shared_ptr<Value::Map_type> map) {
  v8::Local<v8::ObjectTemplate> templ =
      v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _map_template);
  if (!templ.IsEmpty()) {
    v8::Local<v8::Object> self(
        templ->NewInstance(_context->context()).ToLocalChecked());
    if (!self.IsEmpty()) {
      const auto holder =
          new Map_collectable(map, _context->isolate(), self, _context);

      self->SetAlignedPointerInInternalField(0, &magic_pointer);
      self->SetAlignedPointerInInternalField(1, holder);
    }
    return self;
  }
  return {};
}

void JScript_map_wrapper::handler_getter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  const auto collectable = static_cast<Map_collectable *>(
      info.Holder()->GetAlignedPointerFromInternalField(1));
  const auto context = collectable->context();
  const auto &map = collectable->data();

  if (!map) {
    info.GetIsolate()->ThrowException(
        v8_string(info.GetIsolate(), "Reference to invalid object"));
    return;
  }

  const auto prop = to_string(info.GetIsolate(), property);

  {
    Value::Map_type::const_iterator iter = map->find(prop);
    if (iter != map->end())
      info.GetReturnValue().Set(context->convert(iter->second));
  }
}

void JScript_map_wrapper::handler_deleter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Boolean> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Local<v8::Object> obj(info.Holder());
  const auto &map =
      static_cast<Map_collectable *>(obj->GetAlignedPointerFromInternalField(1))
          ->data();
  if (!map) {
    info.GetIsolate()->ThrowException(
        v8_string(info.GetIsolate(), "Reference to invalid object"));
    return;
  }

  const auto prop = to_string(info.GetIsolate(), property);

  map->erase(prop);

  info.GetReturnValue().Set(true);
}

void JScript_map_wrapper::handler_setter(
    v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<v8::Value> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  const auto collectable = static_cast<Map_collectable *>(
      info.Holder()->GetAlignedPointerFromInternalField(1));
  const auto context = collectable->context();
  const auto &map = collectable->data();

  if (!map) {
    info.GetIsolate()->ThrowException(
        v8_string(info.GetIsolate(), "Reference to invalid object"));
    return;
  }

  const auto prop = to_string(info.GetIsolate(), property);
  (*map)[prop] = context->convert(value);

  info.GetReturnValue().Set(value);
}

void JScript_map_wrapper::handler_enumerator(
    const v8::PropertyCallbackInfo<v8::Array> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Local<v8::Object> obj(info.Holder());
  const auto &map =
      static_cast<Map_collectable *>(obj->GetAlignedPointerFromInternalField(1))
          ->data();

  if (!map) throw std::logic_error("bug!");

  v8::Local<v8::Array> marray = v8::Array::New(info.GetIsolate());
  int i = 0;
  for (const auto &iter : *map) {
    marray
        ->Set(info.GetIsolate()->GetCurrentContext(), i++,
              v8_string(info.GetIsolate(), iter.first))
        .FromJust();
  }
  info.GetReturnValue().Set(marray);
}

bool JScript_map_wrapper::is_map(v8::Local<v8::Object> value) {
  if (value->InternalFieldCount() == 2 &&
      value->GetAlignedPointerFromInternalField(0) == (void *)&magic_pointer) {
    return true;
  }
  return false;
}

bool JScript_map_wrapper::unwrap(v8::Local<v8::Object> value,
                                 std::shared_ptr<Value::Map_type> *ret_object) {
  if (is_map(value)) {
    const auto &object = static_cast<Map_collectable *>(
                             value->GetAlignedPointerFromInternalField(1))
                             ->data();
    *ret_object = object;
    return true;
  }
  return false;
}

}  // namespace shcore

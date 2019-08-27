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

#include "scripting/jscript_array_wrapper.h"

#include "mysqlshdk/include/scripting/jscript_collectable.h"
#include "mysqlshdk/include/scripting/jscript_iterator.h"
#include "scripting/jscript_context.h"

#include <iostream>

namespace shcore {

namespace {

static int magic_pointer = 0;

struct Array_config;

using Array_collectable = Collectable<Value::Array_type, Array_config>;

struct Array_config {
 public:
  static size_t get_allocated_size(const Array_collectable &c) {
    return 1024 * c.data()->size();
  }
};

}  // namespace

JScript_array_wrapper::JScript_array_wrapper(JScript_context *context)
    : _context(context) {
  const auto isolate = _context->isolate();
  v8::Local<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(isolate);
  _array_template.Reset(isolate, templ);

  {
    v8::IndexedPropertyHandlerConfiguration config;
    config.getter = &JScript_array_wrapper::handler_igetter;
    config.enumerator = &JScript_array_wrapper::handler_ienumerator;
    templ->SetHandler(config);
  }
  {
    v8::NamedPropertyHandlerConfiguration config;
    config.getter = &JScript_array_wrapper::handler_getter;
    config.flags = v8::PropertyHandlerFlags::kOnlyInterceptStrings;
    templ->SetHandler(config);
  }

  add_iterator<Array_collectable>(templ, isolate);
  templ->SetInternalFieldCount(2);
}

JScript_array_wrapper::~JScript_array_wrapper() { _array_template.Reset(); }

v8::Local<v8::Object> JScript_array_wrapper::wrap(
    std::shared_ptr<Value::Array_type> array) {
  v8::Local<v8::ObjectTemplate> templ =
      v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _array_template);

  v8::Local<v8::Object> obj(
      templ->NewInstance(_context->context()).ToLocalChecked());

  const auto holder =
      new Array_collectable(array, _context->isolate(), obj, _context);

  obj->SetAlignedPointerInInternalField(0, &magic_pointer);
  obj->SetAlignedPointerInInternalField(1, holder);

  return obj;
}

void JScript_array_wrapper::handler_getter(
    v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value> &info) {
  const auto isolate = info.GetIsolate();
  v8::HandleScope hscope(isolate);
  v8::Local<v8::Object> obj(info.Holder());
  const auto &array = static_cast<Array_collectable *>(
                          obj->GetAlignedPointerFromInternalField(1))
                          ->data();

  if (!array) throw std::logic_error("bug!");

  const auto prop = to_string(isolate, property);

  if (prop == "length") {
    info.GetReturnValue().Set(v8::Integer::New(isolate, array->size()));
  }
}

void JScript_array_wrapper::handler_igetter(
    uint32_t index, const v8::PropertyCallbackInfo<v8::Value> &info) {
  v8::HandleScope hscope(info.GetIsolate());

  const auto collectable = static_cast<Array_collectable *>(
      info.Holder()->GetAlignedPointerFromInternalField(1));
  const auto context = collectable->context();
  const auto &array = collectable->data();

  if (!array) throw std::logic_error("bug!");

  if (index < array->size()) {
    info.GetReturnValue().Set(
        context->shcore_value_to_v8_value(array->at(index)));
  }
}

void JScript_array_wrapper::handler_ienumerator(
    const v8::PropertyCallbackInfo<v8::Array> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Local<v8::Object> obj(info.Holder());
  const auto &array = static_cast<Array_collectable *>(
                          obj->GetAlignedPointerFromInternalField(1))
                          ->data();

  v8::Local<v8::Array> r = v8::Array::New(info.GetIsolate(), array->size());
  for (size_t i = 0, c = array->size(); i < c; i++) {
    r->Set(info.GetIsolate()->GetCurrentContext(), i,
           v8::Integer::New(info.GetIsolate(), i))
        .FromJust();
  }
  info.GetReturnValue().Set(r);
}

bool JScript_array_wrapper::unwrap(
    v8::Local<v8::Object> value,
    std::shared_ptr<Value::Array_type> *ret_object) {
  if (value->InternalFieldCount() == 2 &&
      value->GetAlignedPointerFromInternalField(0) == (void *)&magic_pointer) {
    const auto &array = static_cast<Array_collectable *>(
                            value->GetAlignedPointerFromInternalField(1))
                            ->data();
    *ret_object = array;
    return true;
  }
  return false;
}

}  // namespace shcore

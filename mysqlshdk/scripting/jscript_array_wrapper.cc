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

#include "scripting/jscript_array_wrapper.h"

#include "mysqlshdk/include/scripting/jscript_collectable.h"
#include "scripting/jscript_context.h"

#include <iostream>

namespace shcore {

namespace {

static int magic_pointer = 0;

using Array_collectable = Collectable<Value::Array_type>;

}  // namespace

JScript_array_wrapper::JScript_array_wrapper(JScript_context *context)
    : _context(context) {
  v8::Handle<v8::ObjectTemplate> templ =
      v8::ObjectTemplate::New(_context->isolate());
  _array_template.Reset(_context->isolate(), templ);

  {
    // v8::IndexedPropertyHandlerConfiguration config;
    // config.getter = &JScript_array_wrapper::handler_igetter;
    // config.enumerator = &JScript_array_wrapper::handler_ienumerator;
    // templ->SetHandler(config);
    templ->SetIndexedPropertyHandler(
        &JScript_array_wrapper::handler_igetter, 0, 0, 0,
        &JScript_array_wrapper::handler_ienumerator);
  }
  {
    // v8::NamedPropertyHandlerConfiguration config;
    // config.getter = &JScript_array_wrapper::handler_getter;
    // templ->SetHandler(config);
    templ->SetNamedPropertyHandler(&JScript_array_wrapper::handler_getter);
  }

  templ->SetInternalFieldCount(3);
}

JScript_array_wrapper::~JScript_array_wrapper() { _array_template.Reset(); }

v8::Handle<v8::Object> JScript_array_wrapper::wrap(
    std::shared_ptr<Value::Array_type> array) {
  v8::Handle<v8::ObjectTemplate> templ =
      v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _array_template);

  v8::Handle<v8::Object> obj(templ->NewInstance());

  const auto holder = new Array_collectable(array, _context->isolate(), obj);

  obj->SetAlignedPointerInInternalField(0, &magic_pointer);
  obj->SetAlignedPointerInInternalField(1, holder);
  obj->SetAlignedPointerInInternalField(2, this);

  return obj;
}

void JScript_array_wrapper::handler_getter(
    v8::Local<v8::String> property,
    const v8::PropertyCallbackInfo<v8::Value> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  const auto &array = static_cast<Array_collectable *>(
                          obj->GetAlignedPointerFromInternalField(1))
                          ->data();
  //  JScript_array_wrapper *self =
  //  static_cast<JScript_array_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  if (!array) throw std::logic_error("bug!");

  std::string prop = *v8::String::Utf8Value(property);

  /*if (prop == "__members__")
  {
  v8::Handle<v8::Array> marray = v8::Array::New(info.GetIsolate());
  marray->Set(0, v8::String::NewFromUtf8(info.GetIsolate(), "length"));
  info.GetReturnValue().Set(marray);
  }
  else*/ if (prop == "length") {
    info.GetReturnValue().Set(
        v8::Integer::New(info.GetIsolate(), array->size()));
  }
}

void JScript_array_wrapper::handler_igetter(
    uint32_t index, const v8::PropertyCallbackInfo<v8::Value> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  const auto &array = static_cast<Array_collectable *>(
                          obj->GetAlignedPointerFromInternalField(1))
                          ->data();
  JScript_array_wrapper *self = static_cast<JScript_array_wrapper *>(
      obj->GetAlignedPointerFromInternalField(2));

  if (!array) throw std::logic_error("bug!");

  if (index < array->size()) {
    info.GetReturnValue().Set(
        self->_context->shcore_value_to_v8_value(array->at(index)));
  }
}

void JScript_array_wrapper::handler_ienumerator(
    const v8::PropertyCallbackInfo<v8::Array> &info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  const auto &array = static_cast<Array_collectable *>(
                          obj->GetAlignedPointerFromInternalField(1))
                          ->data();

  v8::Handle<v8::Array> r = v8::Array::New(info.GetIsolate(), array->size());
  for (size_t i = 0, c = array->size(); i < c; i++)
    r->Set(i, v8::Integer::New(info.GetIsolate(), i));
  info.GetReturnValue().Set(r);
}

bool JScript_array_wrapper::unwrap(
    v8::Handle<v8::Object> value,
    std::shared_ptr<Value::Array_type> &ret_object) {
  if (value->InternalFieldCount() == 3 &&
      value->GetAlignedPointerFromInternalField(0) == (void *)&magic_pointer) {
    const auto &array = static_cast<Array_collectable *>(
                            value->GetAlignedPointerFromInternalField(1))
                            ->data();
    ret_object = array;
    return true;
  }
  return false;
}

}  // namespace shcore

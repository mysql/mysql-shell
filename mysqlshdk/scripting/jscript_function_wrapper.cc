/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/jscript_function_wrapper.h"
#include "scripting/jscript_context.h"

#include <iostream>

using namespace shcore;

static int magic_pointer = 0;

JScript_function_wrapper::JScript_function_wrapper(JScript_context *context)
  : _context(context) {
  v8::Handle<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(_context->isolate());
  _object_template.Reset(_context->isolate(), templ);
  templ->SetInternalFieldCount(3);
  templ->SetCallAsFunctionHandler(call);
}

JScript_function_wrapper::~JScript_function_wrapper() {
  _object_template.Reset();
}

struct shcore::JScript_function_wrapper::Collectable {
  std::shared_ptr<Function_base> data;
  v8::Persistent<v8::Object> handle;
};

v8::Handle<v8::Object> JScript_function_wrapper::wrap(std::shared_ptr<Function_base> function) {
  v8::Handle<v8::Object> obj(v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _object_template)->NewInstance());
  if (!obj.IsEmpty()) {
    obj->SetAlignedPointerInInternalField(0, &magic_pointer);
    Collectable *tmp = new Collectable();
    tmp->data = function;
    obj->SetAlignedPointerInInternalField(1, tmp);
    obj->SetAlignedPointerInInternalField(2, this);

    // marks the persistent instance to be garbage collectable, with a callback called on deletion
    tmp->handle.Reset(_context->isolate(), v8::Persistent<v8::Object>(_context->isolate(), obj));
    tmp->handle.SetWeak(tmp, wrapper_deleted);
    tmp->handle.MarkIndependent();
  }
  return obj;
}

void JScript_function_wrapper::wrapper_deleted(const v8::WeakCallbackData<v8::Object, Collectable>& data) {
  // the JS wrapper object was deleted, so we also free the shared-ref to the object
  v8::HandleScope hscope(data.GetIsolate());
  data.GetParameter()->data.reset();
  data.GetParameter()->handle.Reset();
  delete data.GetParameter();
}

void JScript_function_wrapper::call(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Handle<v8::Object> obj(args.Holder());
  JScript_function_wrapper *self = static_cast<JScript_function_wrapper *>(
      obj->GetAlignedPointerFromInternalField(2));
  std::shared_ptr<Function_base> shared_ptr_data =
      *static_cast<std::shared_ptr<Function_base> *>(
          obj->GetAlignedPointerFromInternalField(1));
  std::string name = shared_ptr_data->name();
  try {
    Value r = shared_ptr_data->invoke(self->_context->convert_args(args));
    args.GetReturnValue().Set(self->_context->shcore_value_to_v8_value(r));
  } catch (Exception &exc) {
    auto jsexc = self->_context->shcore_value_to_v8_value(Value(exc.error()));
    if (jsexc.IsEmpty())
      jsexc = self->_context->shcore_value_to_v8_value(Value(exc.format()));
    args.GetIsolate()->ThrowException(jsexc);
  } catch (std::exception &exc) {
    args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), exc.what()));
  }
}

bool JScript_function_wrapper::unwrap(v8::Handle<v8::Object> value, std::shared_ptr<Function_base> &ret_object) {
  if (value->InternalFieldCount() == 3 && value->GetAlignedPointerFromInternalField(0) == (void*)&magic_pointer) {
    std::shared_ptr<Function_base> *object = static_cast<std::shared_ptr<Function_base>*>(value->GetAlignedPointerFromInternalField(1));
    ret_object = *object;
    return true;
  }
  return false;
}

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

#include "scripting/jscript_function_wrapper.h"

#include "mysqlshdk/include/scripting/jscript_collectable.h"
#include "scripting/jscript_context.h"

#include <iostream>

namespace shcore {

namespace {

static int magic_pointer = 0;

using Function_collectable = Collectable<Function_base>;

}  // namespace

JScript_function_wrapper::JScript_function_wrapper(JScript_context *context)
    : _context(context) {
  v8::Local<v8::ObjectTemplate> templ =
      v8::ObjectTemplate::New(_context->isolate());
  _object_template.Reset(_context->isolate(), templ);
  templ->SetInternalFieldCount(2);
  templ->SetCallAsFunctionHandler(call);
}

JScript_function_wrapper::~JScript_function_wrapper() {
  _object_template.Reset();
}

v8::Local<v8::Object> JScript_function_wrapper::wrap(
    std::shared_ptr<Function_base> function) {
  v8::Local<v8::Object> obj(
      v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _object_template)
          ->NewInstance(_context->context())
          .ToLocalChecked());
  if (!obj.IsEmpty()) {
    const auto holder =
        new Function_collectable(function, _context->isolate(), obj, _context);

    obj->SetAlignedPointerInInternalField(0, &magic_pointer);
    obj->SetAlignedPointerInInternalField(1, holder);
  }
  return obj;
}

void JScript_function_wrapper::call(
    const v8::FunctionCallbackInfo<v8::Value> &args) {
  const auto collectable = static_cast<Function_collectable *>(
      args.Holder()->GetAlignedPointerFromInternalField(1));
  const auto context = collectable->context();
  const auto &function = collectable->data();

  if (context->is_terminating()) return;

  try {
    Value r = function->invoke(context->convert_args(args));
    args.GetReturnValue().Set(context->convert(r));
  } catch (Exception &exc) {
    auto jsexc = context->convert(Value(exc.error()));
    if (jsexc.IsEmpty()) jsexc = context->convert(Value(exc.format()));
    args.GetIsolate()->ThrowException(jsexc);
  } catch (const std::exception &exc) {
    args.GetIsolate()->ThrowException(v8_string(args.GetIsolate(), exc.what()));
  }
}

bool JScript_function_wrapper::is_function(v8::Local<v8::Object> value) {
  if (value->InternalFieldCount() == 2 &&
      value->GetAlignedPointerFromInternalField(0) == &magic_pointer) {
    return true;
  }
  return false;
}

bool JScript_function_wrapper::unwrap(
    v8::Local<v8::Object> value, std::shared_ptr<Function_base> *ret_object) {
  if (is_function(value)) {
    const auto &object = static_cast<Function_collectable *>(
                             value->GetAlignedPointerFromInternalField(1))
                             ->data();
    *ret_object = object;
    return true;
  }
  return false;
}

}  // namespace shcore

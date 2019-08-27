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

// shcore to JS/v8 type conversion or bridging
#include "scripting/jscript_type_conversion.h"

#include "scripting/jscript_context.h"

#include "scripting/jscript_array_wrapper.h"
#include "scripting/jscript_function_wrapper.h"
#include "scripting/jscript_map_wrapper.h"
#include "scripting/jscript_object_wrapper.h"
#include "scripting/types_jscript.h"

#include "scripting/obj_date.h"

#include <cerrno>
#include <fstream>
#include "utils/utils_string.h"

#include <iostream>

using namespace shcore;

JScript_type_bridger::JScript_type_bridger(JScript_context *context)
    : owner(context),
      object_wrapper(NULL),
      indexed_object_wrapper(NULL),
      function_wrapper(NULL),
      map_wrapper(NULL),
      array_wrapper(NULL) {}

void JScript_type_bridger::init() {
  object_wrapper = new JScript_object_wrapper(owner);
  indexed_object_wrapper = new JScript_object_wrapper(owner, true);
  map_wrapper = new JScript_map_wrapper(owner);
  array_wrapper = new JScript_array_wrapper(owner);
  function_wrapper = new JScript_function_wrapper(owner);
}

void JScript_type_bridger::dispose() {
  if (object_wrapper) {
    delete object_wrapper;
    object_wrapper = NULL;
  }

  if (indexed_object_wrapper) {
    delete indexed_object_wrapper;
    indexed_object_wrapper = NULL;
  }

  if (function_wrapper) {
    delete function_wrapper;
    function_wrapper = NULL;
  }

  if (map_wrapper) {
    delete map_wrapper;
    map_wrapper = NULL;
  }

  if (array_wrapper) {
    delete array_wrapper;
    array_wrapper = NULL;
  }
}

JScript_type_bridger::~JScript_type_bridger() { dispose(); }

double JScript_type_bridger::call_num_method(v8::Local<v8::Object> object,
                                             const char *method) {
  const auto lcontext = owner->context();
  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(
      object->Get(lcontext, owner->v8_string(method)).ToLocalChecked());
  v8::Local<v8::Value> result =
      func->Call(lcontext, object, 0, NULL).ToLocalChecked();
  return result->ToNumber(lcontext).ToLocalChecked()->Value();
}

v8::Local<v8::Value> JScript_type_bridger::native_object_to_js(
    Object_bridge_ref object) {
  if (object && object->class_name() == "Date") {
    std::shared_ptr<Date> date = std::static_pointer_cast<Date>(object);
    // The only Date constructor exposed to C++ takes milliseconds, the
    // constructor that parses a date from an string is implemented in
    // Javascript, so it is invoked this way.
    v8::Local<v8::String> source = owner->v8_string(shcore::str_format(
        "new Date(%d, %d, %d, %d, %d, %d, %d)", date->get_year(),
        date->get_month() - 1, date->get_day(), date->get_hour(),
        date->get_min(), date->get_sec(), date->get_usec() / 1000));
    v8::Local<v8::Context> lcontext = owner->context();
    v8::Context::Scope context_scope(lcontext);
    v8::MaybeLocal<v8::Script> script = v8::Script::Compile(lcontext, source);
    v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(lcontext);
    return result.ToLocalChecked();
  }

  return object->is_indexed() ? indexed_object_wrapper->wrap(object)
                              : object_wrapper->wrap(object);
}

Object_bridge_ref JScript_type_bridger::js_object_to_native(
    v8::Local<v8::Object> object) {
  const auto ctorname =
      to_string(object->GetIsolate(), object->GetConstructorName());

  if (ctorname == "Date") {
    int year, month, day, hour, min, msec;
    float sec;
    year = static_cast<int>(call_num_method(object, "getFullYear"));
    month = static_cast<int>(call_num_method(object, "getMonth"));
    day = static_cast<int>(call_num_method(object, "getDate"));
    hour = static_cast<int>(call_num_method(object, "getHours"));
    min = static_cast<int>(call_num_method(object, "getMinutes"));
    sec = static_cast<float>(call_num_method(object, "getSeconds"));
    msec = static_cast<int>(call_num_method(object, "getMilliseconds"));
    return Object_bridge_ref(
        new Date(year, month + 1, day, hour, min, sec, msec * 1000));
  }

  return Object_bridge_ref();
}

Value JScript_type_bridger::v8_value_to_shcore_value(
    const v8::Local<v8::Value> &value) {
  if (value.IsEmpty() || value->IsUndefined()) {
    return Value();
  } else if (value->IsNull()) {
    return Value::Null();
  } else if (value->IsInt32()) {
    return Value(
        (int64_t)value->ToInt32(owner->context()).ToLocalChecked()->Value());
  } else if (value->IsNumber()) {
    return Value(value->ToNumber(owner->context()).ToLocalChecked()->Value());
  } else if (value->IsString()) {
    return Value(owner->to_string(value));
  } else if (value->IsTrue()) {
    return Value(true);
  } else if (value->IsFalse()) {
    return Value(false);
  } else if (value->IsArray()) {
    v8::Array *jsarray = v8::Array::Cast(*value);
    std::shared_ptr<Value::Array_type> array(
        new Value::Array_type(jsarray->Length()));
    const auto lcontext = owner->context();
    for (int32_t c = jsarray->Length(), i = 0; i < c; i++) {
      v8::Local<v8::Value> item(jsarray->Get(lcontext, i).ToLocalChecked());
      (*array)[i] = v8_value_to_shcore_value(item);
    }
    return Value(array);
  } else if (value->IsFunction()) {
    v8::Local<v8::Function> v8_function = v8::Local<v8::Function>::Cast(value);

    std::shared_ptr<shcore::JScript_function> function(
        new shcore::JScript_function(owner, v8_function));
    // throw Exception::logic_error("JS function wrapping not implemented");
    return Value(std::dynamic_pointer_cast<Function_base>(function));
  } else if (value->IsObject()) {
    v8::Local<v8::Object> jsobject =
        value->ToObject(owner->context()).ToLocalChecked();
    std::shared_ptr<Object_bridge> object;
    std::shared_ptr<Value::Map_type> map;
    std::shared_ptr<Value::Array_type> array;
    std::shared_ptr<Function_base> function;

    if (JScript_array_wrapper::unwrap(jsobject, &array)) {
      return Value(array);
    } else if (JScript_map_wrapper::unwrap(jsobject, &map)) {
      return Value(map);
    } else if (JScript_object_wrapper::unwrap(jsobject, &object)) {
      return Value(object);
    } else if (JScript_function_wrapper::unwrap(jsobject, &function)) {
      return Value(function);
    } else {
      Object_bridge_ref object_ref = js_object_to_native(jsobject);
      if (object_ref) return Value(object_ref);

      const auto lcontext = owner->context();
      v8::Local<v8::Array> pnames(
          jsobject->GetPropertyNames(lcontext).ToLocalChecked());
      std::shared_ptr<Value::Map_type> map_ptr(new Value::Map_type);
      for (int32_t c = pnames->Length(), i = 0; i < c; i++) {
        v8::Local<v8::Value> k(pnames->Get(lcontext, i).ToLocalChecked());
        v8::Local<v8::Value> v(jsobject->Get(lcontext, k).ToLocalChecked());
        (*map_ptr)[owner->to_string(k)] = v8_value_to_shcore_value(v);
      }
      return Value(map_ptr);
    }
  } else if (value->IsSymbol()) {
    return Value(owner->to_string(value));
  } else {
    throw std::invalid_argument("Cannot convert JS value to internal value: " +
                                owner->to_string(value));
  }
  return Value();
}

v8::Local<v8::Value> JScript_type_bridger::shcore_value_to_v8_value(
    const Value &value) {
  v8::Local<v8::Value> r;
  switch (value.type) {
    case Undefined:
      break;
    case Null:
      r = v8::Null(owner->isolate());
      break;
    case Bool:
      r = v8::Boolean::New(owner->isolate(), value.value.b);
      break;
    case String:
      r = owner->v8_string(*value.value.s);
      break;
    case Integer:
      r = v8::Integer::New(owner->isolate(), value.value.i);
      break;
    case UInteger:
      r = v8::Number::New(owner->isolate(), value.value.ui);
      break;
    case Float:
      r = v8::Number::New(owner->isolate(), value.value.d);
      break;
    case Object:
      r = native_object_to_js(*value.value.o);
      break;
    case Array:
      // maybe convert fully
      r = array_wrapper->wrap(*value.value.array);
      break;
    case Map:
      // maybe convert fully
      // r = native_map_to_js(*value.value.map);
      r = map_wrapper->wrap(*value.value.map);
      break;
    case MapRef: {
      std::shared_ptr<Value::Map_type> map(value.value.mapref->lock());
      if (map) {
        throw std::invalid_argument(
            "Cannot convert internal value to JS: wrapmapref not "
            "implemented\n");
      }
    } break;
    case shcore::Function:
      r = function_wrapper->wrap(*value.value.func);
      break;
  }
  return r;
}

v8::Local<v8::String> JScript_type_bridger::type_info(
    v8::Local<v8::Value> value) {
  if (value->IsUndefined())
    return owner->v8_string("Undefined");
  else if (value->IsNull())
    return owner->v8_string("Null");
  else if (value->IsInt32())
    return owner->v8_string("Integer");
  else if (value->IsNumber())
    return owner->v8_string("Number");
  else if (value->IsString())
    return owner->v8_string("String");
  else if (value->IsTrue())
    return owner->v8_string("Bool");
  else if (value->IsFalse())
    return owner->v8_string("Bool");
  else if (value->IsArray())
    return owner->v8_string("Array");
  else if (value->IsFunction())
    return owner->v8_string("Function");
  else if (value->IsObject())  // JS object
  {
    v8::Local<v8::Object> jsobject =
        value->ToObject(owner->context()).ToLocalChecked();
    std::shared_ptr<Object_bridge> object;
    std::shared_ptr<Value::Map_type> map;
    std::shared_ptr<Value::Array_type> array;
    std::shared_ptr<Function_base> function;

    if (JScript_array_wrapper::unwrap(jsobject, &array)) {
      return owner->v8_string("m.Array");
    } else if (JScript_map_wrapper::unwrap(jsobject, &map)) {
      return owner->v8_string("m.Map");
    } else if (JScript_object_wrapper::unwrap(jsobject, &object)) {
      return owner->v8_string("m." + object->class_name());
    } else if (JScript_function_wrapper::unwrap(jsobject, &function)) {
      return owner->v8_string("m.Function");
    } else if (JScript_object_wrapper::is_method(jsobject)) {
      return owner->v8_string("m.Function");
    } else {
      if (!jsobject->GetConstructorName().IsEmpty())
        return jsobject->GetConstructorName();
      else
        return owner->v8_string("Object");
    }
  }
  return v8::Local<v8::String>();
}

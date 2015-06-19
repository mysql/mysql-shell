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

// shcore to JS/v8 type conversion or bridging
#include "shellcore/jscript_type_conversion.h"

#include "shellcore/jscript_context.h"
#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"

#include "shellcore/jscript_object_wrapper.h"
#include "shellcore/jscript_function_wrapper.h"
#include "shellcore/jscript_map_wrapper.h"
#include "shellcore/jscript_array_wrapper.h"

#include "shellcore/obj_date.h"

#include <fstream>
#include <cerrno>
#include <boost/weak_ptr.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>

using namespace shcore;
using namespace boost::system;

JScript_type_bridger::JScript_type_bridger(JScript_context *context)
: owner(context)
{
}

void JScript_type_bridger::init()
{
  object_wrapper = new JScript_object_wrapper(owner);
  map_wrapper = new JScript_map_wrapper(owner);
  array_wrapper = new JScript_array_wrapper(owner);
  function_wrapper = new JScript_function_wrapper(owner);
}

JScript_type_bridger::~JScript_type_bridger()
{
  delete object_wrapper;
  delete function_wrapper;
  delete map_wrapper;
  delete array_wrapper;
}

double JScript_type_bridger::call_num_method(v8::Handle<v8::Object> object, const char *method)
{
  v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(object->Get(v8::String::NewFromUtf8(owner->isolate(), method)));
  v8::Handle<v8::Value> result = func->Call(object, 0, NULL);
  return result->ToNumber()->Value();
}

v8::Handle<v8::Value> JScript_type_bridger::native_object_to_js(Object_bridge_ref object)
{
  if (object && object->class_name() == "Date")
  {
    boost::shared_ptr<Date> date = boost::static_pointer_cast<Date>(object);
    return v8::Date::New(owner->isolate(), date->as_ms());
  }

  return object_wrapper->wrap(object);
}

Object_bridge_ref JScript_type_bridger::js_object_to_native(v8::Handle<v8::Object> object)
{
  std::string ctorname = *v8::String::Utf8Value(object->GetConstructorName());

  if (ctorname == "Date")
  {
    return Date::from_ms((int64_t)call_num_method(object, "getTime"));
  }

  return Object_bridge_ref();
}

Value JScript_type_bridger::v8_value_to_shcore_value(const v8::Handle<v8::Value> &value)
{
  if (value->IsUndefined())
    return Value();
  else if (value->IsNull())
    return Value::Null();
  else if (value->IsInt32())
    return Value((int64_t)value->ToInt32()->Value());
  else if (value->IsNumber())
    return Value(value->ToNumber()->Value());
  else if (value->IsString())
  {
    v8::String::Utf8Value s(value->ToString());
    return Value(*s, s.length());
  }
  else if (value->IsTrue())
    return Value(true);
  else if (value->IsFalse())
    return Value(false);
  else if (value->IsArray())
  {
    v8::Array *jsarray = v8::Array::Cast(*value);
    boost::shared_ptr<Value::Array_type> array(new Value::Array_type(jsarray->Length()));
    for (int32_t c = jsarray->Length(), i = 0; i < c; i++)
    {
      v8::Local<v8::Value> item(jsarray->Get(i));
      (*array)[i] = v8_value_to_shcore_value(item);
    }
    return Value(array);
  }
  else if (value->IsFunction())
  {
    //throw Exception::logic_error("JS function wrapping not implemented");
    return Value::Null();
  }
  else if (value->IsObject()) // JS object
  {
    v8::Handle<v8::Object> jsobject = value->ToObject();
    boost::shared_ptr<Object_bridge> object;
    boost::shared_ptr<Value::Map_type> map;
    boost::shared_ptr<Value::Array_type> array;
    boost::shared_ptr<Function_base> function;

    if (JScript_array_wrapper::unwrap(jsobject, array))
    {
      return Value(array);
    }
    else if (JScript_map_wrapper::unwrap(jsobject, map))
    {
      return Value(map);
    }
    else if (JScript_object_wrapper::unwrap(jsobject, object))
    {
      return Value(object);
    }
    else if (JScript_function_wrapper::unwrap(jsobject, function))
    {
      return Value(function);
    }
    else
    {
      Object_bridge_ref object = js_object_to_native(jsobject);
      if (object)
        return Value(object);

      v8::Local<v8::Array> pnames(jsobject->GetPropertyNames());
      boost::shared_ptr<Value::Map_type> map(new Value::Map_type);
      for (int32_t c = pnames->Length(), i = 0; i < c; i++)
      {
        v8::Local<v8::Value> k(pnames->Get(i));
        v8::Local<v8::Value> v(jsobject->Get(k));
        v8::String::Utf8Value kstr(k);
        (*map)[*kstr] = v8_value_to_shcore_value(v);
      }
      return Value(map);
    }
  }
  else
  {
    v8::String::Utf8Value s(value->ToString());
    throw std::invalid_argument("Cannot convert JS value to internal value: " + std::string(*s));
  }
  return Value();
}

v8::Handle<v8::Value> JScript_type_bridger::shcore_value_to_v8_value(const Value &value)
{
  v8::Handle<v8::Value> r;
  switch (value.type)
  {
    case Undefined:
      break;
    case Null:
      r = v8::Null(owner->isolate());
      break;
    case Bool:
      r = v8::Boolean::New(owner->isolate(), value.value.b);
      break;
    case String:
      r = v8::String::NewFromUtf8(owner->isolate(), value.value.s->c_str());
      break;
    case Integer:
      r = v8::Integer::New(owner->isolate(), value.value.i);
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
      r = map_wrapper->wrap(*value.value.map);
      break;
    case MapRef:
    {
                 boost::shared_ptr<Value::Map_type> map(value.value.mapref->lock());
                 if (map)
                 {
                   std::cout << "wrapmapref not implemented\n";
                 }
    }
      break;
    case shcore::Function:
      r = function_wrapper->wrap(*value.value.func);
      break;
  }
  return r;
}

v8::Handle<v8::String> JScript_type_bridger::type_info(v8::Handle<v8::Value> value)
{
  if (value->IsUndefined())
    return v8::String::NewFromUtf8(owner->isolate(), "Undefined");
  else if (value->IsNull())
    return v8::String::NewFromUtf8(owner->isolate(), "Null");
  else if (value->IsInt32())
    return v8::String::NewFromUtf8(owner->isolate(), "Integer");
  else if (value->IsNumber())
    return v8::String::NewFromUtf8(owner->isolate(), "Number");
  else if (value->IsString())
    return v8::String::NewFromUtf8(owner->isolate(), "String");
  else if (value->IsTrue())
    return v8::String::NewFromUtf8(owner->isolate(), "Bool");
  else if (value->IsFalse())
    return v8::String::NewFromUtf8(owner->isolate(), "Bool");
  else if (value->IsArray())
    return v8::String::NewFromUtf8(owner->isolate(), "Array");
  else if (value->IsFunction())
    return v8::String::NewFromUtf8(owner->isolate(), "Function");
  else if (value->IsObject()) // JS object
  {
    v8::Handle<v8::Object> jsobject = value->ToObject();
    boost::shared_ptr<Object_bridge> object;
    boost::shared_ptr<Value::Map_type> map;
    boost::shared_ptr<Value::Array_type> array;
    boost::shared_ptr<Function_base> function;

    if (JScript_array_wrapper::unwrap(jsobject, array))
      return v8::String::NewFromUtf8(owner->isolate(), "m.Array");
    else if (JScript_map_wrapper::unwrap(jsobject, map))
      return v8::String::NewFromUtf8(owner->isolate(), "m.Map");
    else if (JScript_object_wrapper::unwrap(jsobject, object))
      return v8::String::NewFromUtf8(owner->isolate(), ("m." + object->class_name()).c_str());
    else if (JScript_function_wrapper::unwrap(jsobject, function))
      return v8::String::NewFromUtf8(owner->isolate(), "m.Function");
    else
    {
      if (!jsobject->GetConstructorName().IsEmpty())
        return jsobject->GetConstructorName();
      else
        return v8::String::NewFromUtf8(owner->isolate(), "Object");
    }
  }
  return v8::Handle<v8::String>();
}
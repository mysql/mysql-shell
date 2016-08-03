/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/jscript_array_wrapper.h"
#include "shellcore/jscript_context.h"

#include <iostream>

using namespace shcore;



static int magic_pointer = 0;

JScript_array_wrapper::JScript_array_wrapper(JScript_context *context)
: _context(context)
{
  v8::Handle<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(_context->isolate());
  _array_template.Reset(_context->isolate(), templ);

  {
    //v8::IndexedPropertyHandlerConfiguration config;
    //config.getter = &JScript_array_wrapper::handler_igetter;
    //config.enumerator = &JScript_array_wrapper::handler_ienumerator;
    //templ->SetHandler(config);
    templ->SetIndexedPropertyHandler(&JScript_array_wrapper::handler_igetter, 0, 0, 0, &JScript_array_wrapper::handler_ienumerator);
  }
  {
    //v8::NamedPropertyHandlerConfiguration config;
    //config.getter = &JScript_array_wrapper::handler_getter;
    //templ->SetHandler(config);
    templ->SetNamedPropertyHandler(&JScript_array_wrapper::handler_getter);
  }

  templ->SetInternalFieldCount(3);
}


JScript_array_wrapper::~JScript_array_wrapper()
{
  _array_template.Reset();
}


struct shcore::JScript_array_wrapper::Collectable
{
  std::shared_ptr<Value::Array_type> data;
  v8::Persistent<v8::Object> handle;
};


v8::Handle<v8::Object> JScript_array_wrapper::wrap(std::shared_ptr<Value::Array_type> array)
{
  v8::Handle<v8::ObjectTemplate> templ = v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _array_template);

  v8::Handle<v8::Object> obj(templ->NewInstance());

  obj->SetAlignedPointerInInternalField(0, &magic_pointer);

  Collectable *tmp = new Collectable();
  tmp->data = array;
  obj->SetAlignedPointerInInternalField(1, tmp);
  obj->SetAlignedPointerInInternalField(2, this);

  // marks the persistent instance to be garbage collectable, with a callback called on deletion
  tmp->handle.Reset(_context->isolate(), v8::Persistent<v8::Object>(_context->isolate(), obj));
  tmp->handle.SetWeak(tmp, wrapper_deleted);
  tmp->handle.MarkIndependent();

  return obj;
}


void JScript_array_wrapper::wrapper_deleted(const v8::WeakCallbackData<v8::Object, Collectable>& data)
{
  // the JS wrapper object was deleted, so we also free the shared-ref to the object
  v8::HandleScope hscope(data.GetIsolate());
  data.GetParameter()->data.reset();
  data.GetParameter()->handle.Reset();
  delete data.GetParameter();
}


void JScript_array_wrapper::handler_getter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Value::Array_type> *array = static_cast<std::shared_ptr<Value::Array_type>*>(obj->GetAlignedPointerFromInternalField(1));
//  JScript_array_wrapper *self = static_cast<JScript_array_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  if (!array)
    throw std::logic_error("bug!");

  std::string prop = *v8::String::Utf8Value(property);

  /*if (prop == "__members__")
  {
    v8::Handle<v8::Array> marray = v8::Array::New(info.GetIsolate());
    marray->Set(0, v8::String::NewFromUtf8(info.GetIsolate(), "length"));
    info.GetReturnValue().Set(marray);
  }
  else*/ if (prop == "length")
  {
    info.GetReturnValue().Set(v8::Integer::New(info.GetIsolate(), (*array)->size()));
  }
}


void JScript_array_wrapper::handler_igetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Value::Array_type> *array = static_cast<std::shared_ptr<Value::Array_type>*>(obj->GetAlignedPointerFromInternalField(1));
  JScript_array_wrapper *self = static_cast<JScript_array_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  if (!array)
    throw std::logic_error("bug!");

  if (index < (*array)->size())
  {
    info.GetReturnValue().Set(self->_context->shcore_value_to_v8_value((*array)->at(index)));
  }
}


void JScript_array_wrapper::handler_ienumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Value::Array_type> *array = static_cast<std::shared_ptr<Value::Array_type>*>(obj->GetAlignedPointerFromInternalField(1));

  v8::Handle<v8::Array> r = v8::Array::New(info.GetIsolate(), (*array)->size());
  for (size_t i = 0, c = (*array)->size(); i < c; i++)
    r->Set(i, v8::Integer::New(info.GetIsolate(), i));
  info.GetReturnValue().Set(r);
}


bool JScript_array_wrapper::unwrap(v8::Handle<v8::Object> value, std::shared_ptr<Value::Array_type> &ret_object)
{
  if (value->InternalFieldCount() == 3 && value->GetAlignedPointerFromInternalField(0) == (void*)&magic_pointer)
  {
    std::shared_ptr<Value::Array_type> *object = static_cast<std::shared_ptr<Value::Array_type>*>(value->GetAlignedPointerFromInternalField(1));
    ret_object = *object;
    return true;
  }
  return false;
}

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

#include "scripting/jscript_object_wrapper.h"
#include "scripting/jscript_context.h"

#include <boost/format.hpp>
#include <iostream>

using namespace shcore;

static int magic_pointer = 0;

JScript_object_wrapper::JScript_object_wrapper(JScript_context *context, bool indexed)
  : _context(context) {
  v8::Handle<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(_context->isolate());
  _object_template.Reset(_context->isolate(), templ);

  templ->SetNamedPropertyHandler(&JScript_object_wrapper::handler_getter, &JScript_object_wrapper::handler_setter, &JScript_object_wrapper::handler_query, 0, &JScript_object_wrapper::handler_enumerator);

  if (indexed)
    templ->SetIndexedPropertyHandler(&JScript_object_wrapper::handler_igetter, &JScript_object_wrapper::handler_isetter, 0, 0, &JScript_object_wrapper::handler_ienumerator);

  templ->SetInternalFieldCount(3);
}

JScript_object_wrapper::~JScript_object_wrapper() {
  _object_template.Reset();
}

struct shcore::JScript_object_wrapper::Collectable {
  std::shared_ptr<Object_bridge> data;
  v8::Persistent<v8::Object> handle;
};

v8::Handle<v8::Object> JScript_object_wrapper::wrap(std::shared_ptr<Object_bridge> object) {
  v8::Handle<v8::ObjectTemplate> templ = v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _object_template);

  v8::Handle<v8::Object> obj(templ->NewInstance());

  obj->SetAlignedPointerInInternalField(0, &magic_pointer);

  Collectable *tmp = new Collectable();
  tmp->data = object;

  obj->SetAlignedPointerInInternalField(1, tmp);
  obj->SetAlignedPointerInInternalField(2, this);

  // marks the persistent instance to be garbage collectable, with a callback called on deletion
  tmp->handle.Reset(_context->isolate(), v8::Persistent<v8::Object>(_context->isolate(), obj));
  tmp->handle.SetWeak(tmp, wrapper_deleted);
  tmp->handle.MarkIndependent();

  return obj;
}

void JScript_object_wrapper::wrapper_deleted(const v8::WeakCallbackData<v8::Object, Collectable>& data) {
  // the JS wrapper object was deleted, so we also free the shared-ref to the object
  v8::HandleScope hscope(data.GetIsolate());
  data.GetParameter()->data.reset();
  data.GetParameter()->handle.Reset();
  delete data.GetParameter();
}

void JScript_object_wrapper::handler_getter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Object_bridge> *object = static_cast<std::shared_ptr<Object_bridge>*>(obj->GetAlignedPointerFromInternalField(1));
  JScript_object_wrapper *self = static_cast<JScript_object_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  if (!object)
    throw std::logic_error("bug!");

  v8::String::Utf8Value prop(property);
  /*if (prop == "__members__")
  {
  std::vector<std::string> members((*object)->get_members());
  v8::Handle<v8::Array> marray = v8::Array::New(info.GetIsolate());
  int i = 0;
  for (std::vector<std::string>::const_iterator iter = members.begin(); iter != members.end(); ++iter, ++i)
  {
  marray->Set(i, v8::String::NewFromUtf8(info.GetIsolate(), iter->c_str()));
  }
  info.GetReturnValue().Set(marray);
  }
  else*/
  if (strcmp(*prop, "length") == 0 && (*object)->has_member("length")) {
    try {
      info.GetReturnValue().Set(self->_context->shcore_value_to_v8_value((*object)->get_member("length")));
      return;
    } catch (Exception &exc) {
      if (!exc.is_attribute())
        info.GetIsolate()->ThrowException(self->_context->shcore_value_to_v8_value(Value(exc.error())));
      // fallthrough
    }
  }
  {
    try {
      Value member = (*object)->get_member(*prop);
      info.GetReturnValue().Set(self->_context->shcore_value_to_v8_value(member));
    } catch (Exception &exc) {
      info.GetIsolate()->ThrowException(self->_context->shcore_value_to_v8_value(Value(exc.error())));
    }
  }
}

void JScript_object_wrapper::handler_setter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Object_bridge> *object = static_cast<std::shared_ptr<Object_bridge>*>(obj->GetAlignedPointerFromInternalField(1));
  JScript_object_wrapper *self = static_cast<JScript_object_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  if (!object)
    throw std::logic_error("bug!");

  v8::String::Utf8Value prop(property);
  try {
    (*object)->set_member(*prop, self->_context->v8_value_to_shcore_value(value));
    info.GetReturnValue().Set(value);
  } catch (Exception &exc) {
    info.GetIsolate()->ThrowException(self->_context->shcore_value_to_v8_value(Value(exc.error())));
  }
}

void JScript_object_wrapper::handler_query(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Integer>& info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Object_bridge> *object = static_cast<std::shared_ptr<Object_bridge>*>(obj->GetAlignedPointerFromInternalField(1));

  if (!object)
    throw std::logic_error("bug!");

  v8::String::Utf8Value prop(property);

  if ((*object)->has_member(*prop)) {
    //v8::Integer::New(info.GetIsolate(), 1);
    info.GetReturnValue().Set(v8::None);
  }
}

void JScript_object_wrapper::handler_enumerator(const v8::PropertyCallbackInfo<v8::Array>& info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Object_bridge> *object = static_cast<std::shared_ptr<Object_bridge>*>(obj->GetAlignedPointerFromInternalField(1));

  if (!object)
    throw std::logic_error("bug!");

  std::vector<std::string> members((*object)->get_members());
  v8::Handle<v8::Array> marray = v8::Array::New(info.GetIsolate());
  int i = 0;
  for (std::vector<std::string>::const_iterator iter = members.begin(); iter != members.end(); ++iter, ++i) {
    marray->Set(i, v8::String::NewFromUtf8(info.GetIsolate(), iter->c_str()));
  }
  info.GetReturnValue().Set(marray);
}

void JScript_object_wrapper::handler_igetter(uint32_t i, const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Object_bridge> *object = static_cast<std::shared_ptr<Object_bridge>*>(obj->GetAlignedPointerFromInternalField(1));
  JScript_object_wrapper *self = static_cast<JScript_object_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  if (!object)
    throw std::logic_error("bug!");

  {
    try {
      Value member = (*object)->get_member(i);
      info.GetReturnValue().Set(self->_context->shcore_value_to_v8_value(member));
    } catch (Exception &exc) {
      info.GetIsolate()->ThrowException(self->_context->shcore_value_to_v8_value(Value(exc.error())));
    }
  }
}

void JScript_object_wrapper::handler_isetter(uint32_t i, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Object_bridge> *object = static_cast<std::shared_ptr<Object_bridge>*>(obj->GetAlignedPointerFromInternalField(1));
  JScript_object_wrapper *self = static_cast<JScript_object_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  if (!object)
    throw std::logic_error("bug!");

  try {
    (*object)->set_member(i, self->_context->v8_value_to_shcore_value(value));
    info.GetReturnValue().Set(value);
  } catch (Exception &exc) {
    info.GetIsolate()->ThrowException(self->_context->shcore_value_to_v8_value(Value(exc.error())));
  }
}

void JScript_object_wrapper::handler_ienumerator(const v8::PropertyCallbackInfo<v8::Array>& info) {
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  std::shared_ptr<Object_bridge> *object = static_cast<std::shared_ptr<Object_bridge>*>(obj->GetAlignedPointerFromInternalField(1));

  if (!object)
    throw std::logic_error("bug!");

  std::vector<std::string> members((*object)->get_members());
  v8::Handle<v8::Array> marray = v8::Array::New(info.GetIsolate());
  int i = 0;
  for (std::vector<std::string>::const_iterator iter = members.begin(); iter != members.end(); ++iter, ++i) {
    uint32_t x = 0;
    if (sscanf(iter->c_str(), "%u", &x) == 1)
        marray->Set(i, v8::Integer::New(info.GetIsolate(), x));
  }
  info.GetReturnValue().Set(marray);
}

bool JScript_object_wrapper::unwrap(v8::Handle<v8::Object> value, std::shared_ptr<Object_bridge> &ret_object) {
  if (value->InternalFieldCount() == 3 && value->GetAlignedPointerFromInternalField(0) == (void*)&magic_pointer) {
    std::shared_ptr<Object_bridge> *object = static_cast<std::shared_ptr<Object_bridge>*>(value->GetAlignedPointerFromInternalField(1));
    ret_object = *object;
    return true;
  }
  return false;
}

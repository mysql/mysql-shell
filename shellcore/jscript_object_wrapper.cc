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

#include "shellcore/jscript_object_wrapper.h"
#include "shellcore/jscript_context.h"

#include <iostream>

using namespace shcore;



static int magic_pointer = 0;

JScript_object_wrapper::JScript_object_wrapper(JScript_context *context)
: _context(context)
{
  v8::Handle<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(_context->isolate());
  _object_template.Reset(_context->isolate(), templ);

  v8::NamedPropertyHandlerConfiguration config;
  config.getter = &JScript_object_wrapper::handler_getter;
  templ->SetHandler(config);

  templ->SetInternalFieldCount(3);
}


JScript_object_wrapper::~JScript_object_wrapper()
{
  _object_template.Reset();
}


v8::Handle<v8::Object> JScript_object_wrapper::wrap(boost::shared_ptr<Object_bridge> object)
{
  v8::Handle<v8::ObjectTemplate> templ = v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _object_template);

  v8::Handle<v8::Object> obj(templ->NewInstance());
  v8::Persistent<v8::Object> persistent(_context->isolate(), obj);

  obj->SetAlignedPointerInInternalField(0, &magic_pointer);

  boost::shared_ptr<Object_bridge> *tmp = new boost::shared_ptr<Object_bridge>(object);
  obj->SetAlignedPointerInInternalField(1, tmp);

  obj->SetAlignedPointerInInternalField(2, this);

  // marks the persistent instance to be garbage collectable, with a callback called on deletion
  persistent.SetWeak(tmp, wrapper_deleted);
  persistent.MarkIndependent();

  return obj;
}


void JScript_object_wrapper::wrapper_deleted(const v8::WeakCallbackData<v8::Object, boost::shared_ptr<Object_bridge>>& data)
{
  // the JS wrapper object was deleted, so we also free the shared-ref to the object
  v8::HandleScope hscope(data.GetIsolate());
  delete data.GetParameter();
}


void JScript_object_wrapper::handler_getter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
  v8::HandleScope hscope(info.GetIsolate());
  v8::Handle<v8::Object> obj(info.Holder());
  boost::shared_ptr<Object_bridge> *object = static_cast<boost::shared_ptr<Object_bridge>*>(obj->GetAlignedPointerFromInternalField(1));
  JScript_object_wrapper *self = static_cast<JScript_object_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  if (!object)
    throw std::logic_error("bug!");

  const char *prop = *v8::String::Utf8Value(property);
  std::cout << "get:" << prop << "\n";
  if (strcmp(prop, "__members__") == 0)
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
  else
  {
    Value member = (*object)->get_member(prop);
    if (!member)
      info.GetIsolate()->ThrowException(v8::String::NewFromUtf8(info.GetIsolate(), (std::string("Invalid member ").append(prop)).c_str()));
    info.GetReturnValue().Set(self->_context->shcore_value_to_v8_value(member));
  }
}


bool JScript_object_wrapper::unwrap(v8::Handle<v8::Object> value, boost::shared_ptr<Object_bridge> &ret_object)
{
  if (value->InternalFieldCount() == 3 && value->GetAlignedPointerFromInternalField(0) == (void*)&magic_pointer)
  {
    boost::shared_ptr<Object_bridge> *object = static_cast<boost::shared_ptr<Object_bridge>*>(value->GetAlignedPointerFromInternalField(1));
    ret_object = *object;
    return true;
  }
  return false;
}

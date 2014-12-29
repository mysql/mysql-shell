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

#include "shellcore/jscript_function_wrapper.h"
#include "shellcore/jscript_context.h"

#include <iostream>

using namespace shcore;



static int magic_pointer = 0;

JScript_function_wrapper::JScript_function_wrapper(JScript_context *context)
: _context(context)
{
  v8::Handle<v8::ObjectTemplate> templ = v8::ObjectTemplate::New(_context->isolate());
  _object_template.Reset(_context->isolate(), templ);
  templ->SetInternalFieldCount(3);
  templ->SetCallAsFunctionHandler(call);
}


JScript_function_wrapper::~JScript_function_wrapper()
{
  _object_template.Reset();
}


v8::Handle<v8::Object> JScript_function_wrapper::wrap(boost::shared_ptr<Function_base> function)
{
  v8::Handle<v8::Object> obj(v8::Local<v8::ObjectTemplate>::New(_context->isolate(), _object_template)->NewInstance());

  obj->SetAlignedPointerInInternalField(0, &magic_pointer);
  boost::shared_ptr<Function_base> *shared_ptr_data = new boost::shared_ptr<Function_base>(function);
  obj->SetAlignedPointerInInternalField(1, shared_ptr_data);
  obj->SetAlignedPointerInInternalField(2, this);

  {
    v8::Persistent<v8::Object> data(_context->isolate(), obj);
    // marks the persistent instance to be garbage collectable, with a callback called on deletion
    data.SetWeak(shared_ptr_data, wrapper_deleted);
    data.MarkIndependent();
  }
  return obj;
}


void JScript_function_wrapper::call(const v8::FunctionCallbackInfo<v8::Value>& args)
{
  v8::Handle<v8::Object> obj(args.Holder());
  JScript_function_wrapper *self = static_cast<JScript_function_wrapper*>(obj->GetAlignedPointerFromInternalField(2));

  boost::shared_ptr<Function_base> *shared_ptr_data = static_cast<boost::shared_ptr<Function_base>*>(obj->GetAlignedPointerFromInternalField(1));

  Value r = (*shared_ptr_data)->invoke(self->_context->convert_args(args));

  args.GetReturnValue().Set(self->_context->shcore_value_to_v8_value(r));
}


void JScript_function_wrapper::wrapper_deleted(const v8::WeakCallbackData<v8::Object, boost::shared_ptr<Function_base>>& data)
{
  // the JS wrapper object was deleted, so we also free the shared-ref to the object
  v8::HandleScope hscope(data.GetIsolate());
  delete data.GetParameter();
}


bool JScript_function_wrapper::unwrap(v8::Handle<v8::Object> value, boost::shared_ptr<Function_base> &ret_object)
{
  if (value->InternalFieldCount() == 3 && value->GetAlignedPointerFromInternalField(0) == (void*)&magic_pointer)
  {
    boost::shared_ptr<Function_base> *object = static_cast<boost::shared_ptr<Function_base>*>(value->GetAlignedPointerFromInternalField(1));
    ret_object = *object;
    return true;
  }
  return false;
}

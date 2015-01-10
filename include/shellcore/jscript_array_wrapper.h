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


#ifndef _JSCRIPT_ARRAY_WRAPPER_H_
#define _JSCRIPT_ARRAY_WRAPPER_H_

#include "shellcore/types.h"
#include <include/v8.h>

namespace shcore
{
class JScript_context;

class JScript_array_wrapper
{
public:
  JScript_array_wrapper(JScript_context *context);
  ~JScript_array_wrapper();

  v8::Handle<v8::Object> wrap(boost::shared_ptr<Value::Array_type> array);

  static bool unwrap(v8::Handle<v8::Object> value, boost::shared_ptr<Value::Array_type> &ret_array);

private:
  static void handler_igetter(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info);
  static void handler_ienumerator(const v8::PropertyCallbackInfo<v8::Array>& info);
  static void handler_getter(v8::Local<v8::Name> prop, const v8::PropertyCallbackInfo<v8::Value>& info);


  static void wrapper_deleted(const v8::WeakCallbackData<v8::Object, boost::shared_ptr<Value::Array_type> >& data);

private:
  JScript_context *_context;
  v8::Persistent<v8::ObjectTemplate> _array_template;
};

};


#endif

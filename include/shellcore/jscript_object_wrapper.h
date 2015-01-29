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


// Provides a generic wrapper for shcore::Object_bridge objects so that they
// can be used from JavaScript

// new_instance() can be used to create a JS object instance per class

#ifndef _JSCRIPT_OBJECT_WRAPPER_H_
#define _JSCRIPT_OBJECT_WRAPPER_H_

#include "shellcore/types.h"
#include <v8.h>

namespace shcore
{
class JScript_context;

class JScript_object_wrapper
{
public:
  JScript_object_wrapper(JScript_context *context);
  ~JScript_object_wrapper();

  v8::Handle<v8::Object> wrap(boost::shared_ptr<Object_bridge> object);

  static bool unwrap(v8::Handle<v8::Object> value, boost::shared_ptr<Object_bridge> &ret_object);

private:
  static void handler_getter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);
  static void handler_setter(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info);
  static void handler_enumerator(const v8::PropertyCallbackInfo<v8::Array>& info);

  static void wrapper_deleted(const v8::WeakCallbackData<v8::Object, boost::shared_ptr<Object_bridge> >& data);

private:
  JScript_context *_context;
  v8::Persistent<v8::ObjectTemplate> _object_template;
};

};


#endif

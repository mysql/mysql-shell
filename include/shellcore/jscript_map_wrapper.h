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


#ifndef _JSCRIPT_MAP_WRAPPER_H_
#define _JSCRIPT_MAP_WRAPPER_H_

#include "shellcore/types.h"
#include <include/v8.h>

namespace shcore
{
class JScript_context;

class JScript_map_wrapper
{
public:
  JScript_map_wrapper(JScript_context *context);
  ~JScript_map_wrapper();

  v8::Handle<v8::Object> wrap(boost::shared_ptr<Value::Map_type> map);

  static bool unwrap(v8::Handle<v8::Object> value, boost::shared_ptr<Value::Map_type> &ret_map);

private:
  static void handler_getter(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info);

  static void wrapper_deleted(const v8::WeakCallbackData<v8::Object, boost::shared_ptr<Value::Map_type> >& data);

private:
  JScript_context *_context;
  v8::Persistent<v8::ObjectTemplate> _map_template;
};

};


#endif

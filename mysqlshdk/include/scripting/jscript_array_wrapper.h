/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _JSCRIPT_ARRAY_WRAPPER_H_
#define _JSCRIPT_ARRAY_WRAPPER_H_

#include <memory>

#include "scripting/include_v8.h"
#include "scripting/types.h"

namespace shcore {
class JScript_context;

class JScript_array_wrapper {
 public:
  JScript_array_wrapper(JScript_context *context);
  ~JScript_array_wrapper();

  v8::Local<v8::Object> wrap(std::shared_ptr<Value::Array_type> array);

  static bool unwrap(v8::Local<v8::Object> value,
                     std::shared_ptr<Value::Array_type> *ret_array);

 private:
  static void handler_igetter(uint32_t index,
                              const v8::PropertyCallbackInfo<v8::Value> &info);
  static void handler_ienumerator(
      const v8::PropertyCallbackInfo<v8::Array> &info);
  static void handler_getter(v8::Local<v8::Name> prop,
                             const v8::PropertyCallbackInfo<v8::Value> &info);

 private:
  JScript_context *_context;
  v8::Persistent<v8::ObjectTemplate> _array_template;
};
};  // namespace shcore

#endif

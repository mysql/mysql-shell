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

#include "scripting/types_jscript.h"
#include "scripting/common.h"
#include "scripting/object_factory.h"

#include <v8.h>

using namespace shcore;

class JScript_object : public Object_bridge {
 public:
  JScript_object(std::shared_ptr<JScript_context> UNUSED(context)) {}

  virtual ~JScript_object() {}

  virtual std::string &append_descr(std::string &s_out, int UNUSED(indent) = -1,
                                    int UNUSED(quote_strings) = 0) const {
    return s_out;
  }

  virtual std::string &append_repr(std::string &s_out) const { return s_out; }

  virtual std::vector<std::string> get_members() const {
    std::vector<std::string> mlist;
    return mlist;
  }

  virtual bool operator==(const Object_bridge &UNUSED(other)) const {
    return false;
  }

  virtual bool operator!=(const Object_bridge &UNUSED(other)) const {
    return false;
  }

  virtual Value get_property(const std::string &UNUSED(prop)) const {
    return Value();
  }

  virtual void set_property(const std::string &UNUSED(prop),
                            Value UNUSED(value)) {}

  virtual Value call(const std::string &name, const Argument_list &args) {
    const auto lcontext = _js->context();
    v8::Context::Scope context_scope(lcontext);
    auto member = _object->Get(lcontext, _js->v8_string(name));
    if (!member.IsEmpty() && member.ToLocalChecked()->IsFunction()) {
      v8::Local<v8::Function> f;
      f.Cast(member.ToLocalChecked());
      std::vector<v8::Local<v8::Value>> argv;

      // XX convert the arg list

      v8::MaybeLocal<v8::Value> result =
          f->Call(lcontext, _js->context()->Global(),
                  static_cast<int>(args.size()), &argv[0]);
      return _js->v8_value_to_shcore_value(result.ToLocalChecked());
    } else {
      throw Exception::attrib_error("Called member " + name +
                                    " of JS object is not a function");
    }
  }

 private:
  std::shared_ptr<JScript_context> _js;
  v8::Local<v8::Object> _object;
};

// -------------------------------------------------------------------------------------------------------

class JScript_object_factory : public Object_factory {
 public:
  JScript_object_factory(std::shared_ptr<JScript_context> context,
                         v8::Local<v8::Object> constructor);

  virtual std::shared_ptr<Object_bridge> construct(
      const Argument_list &UNUSED(args)) {
    return construct_from_repr("");
  }

  virtual std::shared_ptr<Object_bridge> construct_from_repr(
      const std::string &UNUSED(repr)) {
    return std::shared_ptr<Object_bridge>();
  }

 private:
  std::weak_ptr<JScript_context> _context;
};

// -------------------------------------------------------------------------------------------------------

JScript_function::JScript_function(JScript_context *context,
                                   v8::Local<v8::Function> function)
    : _js(context) {
  _function.Reset(_js->isolate(), function);
}

const std::string &JScript_function::name() const {
  // TODO:
  static std::string tmp;
  return tmp;
}

const std::vector<std::pair<std::string, Value_type>>
    &JScript_function::signature() const {
  // TODO:
  static std::vector<std::pair<std::string, Value_type>> tmp;
  return tmp;
}

Value_type JScript_function::return_type() const {
  // TODO:
  return Undefined;
}

bool JScript_function::operator==(const Function_base &UNUSED(other)) const {
  // TODO:
  return false;
}

bool JScript_function::operator!=(const Function_base &UNUSED(other)) const {
  // TODO:
  return false;
}

Value JScript_function::invoke(const Argument_list &args) {
  const unsigned argc = args.size();
  v8::Local<v8::Value> *argv = new v8::Local<v8::Value>[argc];

  for (size_t index = 0; index < args.size(); index++)
    argv[index] = _js->shcore_value_to_v8_value(args[index]);

  v8::Local<v8::Function> callback =
      v8::Local<v8::Function>::New(_js->isolate(), _function);

  v8::Local<v8::Context> lcontext = _js->context();
  v8::Context::Scope context_scope(lcontext);

  v8::MaybeLocal<v8::Value> ret_val = callback->Call(
      lcontext, _js->isolate()->GetCurrentContext()->Global(), argc, argv);

  delete[] argv;

  return _js->v8_value_to_shcore_value(ret_val.ToLocalChecked());
}

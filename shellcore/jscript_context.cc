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

#include "shellcore/jscript_context.h"
#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"
#include "shellcore/jscript_object_wrapper.h"
#include "shellcore/jscript_function_wrapper.h"
#include "shellcore/jscript_map_wrapper.h"
#include "shellcore/jscript_array_wrapper.h"
#include "shellcore/object_factory.h"
#include "shellcore/object_registry.h"

#include <fstream>
#include <cerrno>
#include <boost/weak_ptr.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>

using namespace shcore;
using namespace boost::system;

struct JScript_context::JScript_context_impl
{
  JScript_context *owner;
  v8::Isolate *isolate;
  v8::Persistent<v8::Context> context;
  std::map<std::string, v8::Persistent<v8::Object>* > factory_packages;
  v8::Persistent<v8::ObjectTemplate> package_template;
  JScript_object_wrapper *object_wrapper;
  JScript_function_wrapper *function_wrapper;
  JScript_map_wrapper *map_wrapper;
  JScript_array_wrapper *array_wrapper;

  Interpreter_delegate *delegate;

  JScript_context_impl(JScript_context *owner_, Interpreter_delegate *deleg)
  : owner(owner_), isolate(v8::Isolate::New()), object_wrapper(NULL), function_wrapper(NULL),
    map_wrapper(NULL), array_wrapper(NULL), delegate(deleg)
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Handle<v8::ObjectTemplate> globals = v8::ObjectTemplate::New(isolate);
    v8::Local<v8::External> client_data(v8::External::New(isolate, this));

    // register symbols to be exported to JS in global namespace

    // repr(object) -> string
    globals->Set(v8::String::NewFromUtf8(isolate, "repr"),
                 v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_repr, client_data));

    // unrepr(string) -> object
    globals->Set(v8::String::NewFromUtf8(isolate, "unrepr"),
                 v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_unrepr, client_data));

    // type(object) -> string
    globals->Set(v8::String::NewFromUtf8(isolate, "type"),
                 v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_type, client_data));

    // print('hello')
    globals->Set(v8::String::NewFromUtf8(isolate, "print"),
                 v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_print, client_data));

    // s = input('Type something:')
    globals->Set(v8::String::NewFromUtf8(isolate, "input"),
                 v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_input, client_data));

    // s = input('Password:')
    globals->Set(v8::String::NewFromUtf8(isolate, "password"),
                 v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_password, client_data));

    // obj = _F.mysql.open('root@localhost')
    globals->Set(v8::String::NewFromUtf8(isolate, "_F"), make_factory());

    {
      v8::Handle<v8::ObjectTemplate> templ(v8::ObjectTemplate::New(isolate));
      package_template.Reset(isolate, templ);
      templ->SetNamedPropertyHandler(&JScript_context_impl::factory_package_getter, 0, 0, 0, 0, client_data);
    }

    context.Reset(isolate, v8::Context::New(isolate, NULL, globals));
  }

  ~JScript_context_impl()
  {
    for (std::map<std::string, v8::Persistent<v8::Object>* >::iterator i = factory_packages.begin(); i != factory_packages.end(); ++i)
      delete i->second;
    delete object_wrapper;
    delete function_wrapper;
    delete map_wrapper;
    delete array_wrapper;
  }


  // Factory interface implementation

  static void factory_getter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
  {
    v8::HandleScope outer_handle_scope(info.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*info.Data())->Value());

    std::string pkgname = *v8::String::Utf8Value(property->ToString());

    std::map<std::string, v8::Persistent<v8::Object>* >::iterator iter;
    if ((iter = self->factory_packages.find(pkgname)) == self->factory_packages.end())
    {
      // check if the package exists
      if (Object_factory::has_package(pkgname))
      {
        v8::Handle<v8::Object> package = self->make_factory_package(pkgname);
        info.GetReturnValue().Set(package);
        self->factory_packages[pkgname] = new v8::Persistent<v8::Object>(self->isolate, package);
      }
      else
        info.GetIsolate()->ThrowException(v8::String::NewFromUtf8(info.GetIsolate(), ("Invalid package "+pkgname).c_str()));
    }
    else
    {
      info.GetReturnValue().Set(*iter->second);
    }
  }

  v8::Handle<v8::ObjectTemplate> make_factory()
  {
    v8::Handle<v8::ObjectTemplate> factory = v8::ObjectTemplate::New(isolate);
    v8::Local<v8::External> client_data(v8::External::New(isolate, this));

    factory->SetNamedPropertyHandler(&JScript_context_impl::factory_getter, 0, 0, 0, 0, client_data);

    return factory;
  }

  static void factory_package_getter(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
  {
    v8::HandleScope outer_handle_scope(info.GetIsolate());

    v8::String::Utf8Value propname(property);

    if (strcmp(*propname, "__name__") != 0)
    {
      JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*info.Data())->Value());
      v8::String::Utf8Value pkgname(info.This()->Get(v8::String::NewFromUtf8(self->isolate, "__name__")));

      info.GetReturnValue().Set(self->wrap_factory_constructor(*pkgname, *propname));
    }
  }

  v8::Handle<v8::Object> make_factory_package(const std::string &package)
  {
    v8::Handle<v8::ObjectTemplate> templ(v8::Local<v8::ObjectTemplate>::New(isolate, package_template));
    v8::Handle<v8::Object> pkg = templ->NewInstance();
    pkg->Set(v8::String::NewFromUtf8(isolate, "__name__"), v8::String::NewFromUtf8(isolate, package.c_str()));
    return pkg;
  }

  static void call_factory(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::Handle<v8::Object> client_data(args.Data()->ToObject());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*client_data->Get(v8::String::NewFromUtf8(args.GetIsolate(), "object")))->Value());
    std::string package = *(v8::String::Utf8Value)client_data->Get(v8::String::NewFromUtf8(self->isolate, "package"));
    std::string factory = *(v8::String::Utf8Value)client_data->Get(v8::String::NewFromUtf8(self->isolate, "function"));

    try
    {
      Value result(Object_factory::call_constructor(package, factory, self->convert_args(args)));
      if (result)
        args.GetReturnValue().Set(self->shcore_value_to_v8_value(result));
    }
    catch (std::exception &e)
    {
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), e.what()));
    }
  }

  v8::Handle<v8::Function> wrap_factory_constructor(const std::string &package, const std::string &factory)
  {
    v8::Local<v8::Object> client_data(v8::Object::New(isolate));

    client_data->Set(v8::String::NewFromUtf8(isolate, "package"), v8::String::NewFromUtf8(isolate, package.c_str()));
    client_data->Set(v8::String::NewFromUtf8(isolate, "function"), v8::String::NewFromUtf8(isolate, factory.c_str()));
    client_data->Set(v8::String::NewFromUtf8(isolate, "object"), v8::External::New(isolate, this));

    v8::Handle<v8::FunctionTemplate> ftempl = v8::FunctionTemplate::New(isolate, &JScript_context_impl::call_factory, client_data);
    return ftempl->GetFunction();
  }

  // Global functions exposed to JS

  static void f_repr(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    if (args.Length() != 1)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "repr() takes 1 argument"));
    else
    {
      try
      {
        args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(),
                                  self->v8_value_to_shcore_value(args[0]).repr().c_str()));
      }
      catch (std::exception &e)
      {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), e.what()));
      }
    }
  }

  static void f_unrepr(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    if (args.Length() != 1)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "unrepr() takes 1 argument"));
    else
    {
      try
      {
        v8::String::Utf8Value s(args[0]);
        args.GetReturnValue().Set(self->shcore_value_to_v8_value(Value::parse(*s)));
      }
      catch (std::exception &e)
      {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), e.what()));
      }
    }
  }

  static void f_type(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    if (args.Length() != 1)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "type() takes 1 argument"));
    else
    {
      try
      {
        Value v(self->v8_value_to_shcore_value(args[0]));

        if (v.type == Object)
          args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(),
                                                            ("Object:"+v.as_object<Object_bridge>()->class_name()).c_str()));
        else
          args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(),
                              type_name(v.type).c_str()));
      }
      catch (std::exception &e)
      {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), e.what()));
      }
    }
  }

  static void f_print(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope outer_handle_scope(args.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    for (int i = 0; i < args.Length(); i++)
    {
      v8::HandleScope handle_scope(args.GetIsolate());
      if (i > 0)
        self->delegate->print(self->delegate->user_data, " ");

      try
      {
        self->delegate->print(self->delegate->user_data, self->v8_value_to_shcore_value(args[i]).descr(true).c_str());
      }
      catch (std::exception &e)
      {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), e.what()));
        break;
      }
    }
  }

  static void f_input(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope outer_handle_scope(args.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    if (args.Length() != 1)
    {
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "Invalid number of parameters"));
      return;
    }

    v8::HandleScope handle_scope(args.GetIsolate());
    v8::String::Utf8Value str(args[0]);
    std::string r = self->delegate->input(self->delegate->user_data, *str);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), r.c_str()));
  }

  static void f_password(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope outer_handle_scope(args.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    if (args.Length() != 1)
    {
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "Invalid number of parameters"));
      return;
    }

    v8::HandleScope handle_scope(args.GetIsolate());
    v8::String::Utf8Value str(args[0]);
    std::string r = self->delegate->password(self->delegate->user_data, *str);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), r.c_str()));
  }

  // --------------------
  Argument_list convert_args(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    Argument_list r;

    for (int c = args.Length(), i = 0; i < c; i++)
    {
      r.push_back(v8_value_to_shcore_value(args[i]));
    }
    return r;
  }


  std::string format_exception(const shcore::Value &exc)
  {
    try
    {
      if (exc.type == Map)
      {
        std::string type = exc.as_map()->get_string("type", "");
        std::string message = exc.as_map()->get_string("message", "");
        int64_t code = exc.as_map()->get_int("code", -1);

        if (code < 0)
          return (boost::format("%1%: %2%") % type % message).str();
        else
          return (boost::format("%1%: %2% (%3%)") % type % message % code).str();
      }
    }
    catch (std::exception &e)
    {
      std::cerr << e.what() << ": unexpected format of exception object\n";
    }
    return exc.descr(false);
  }
  

  void print_exception(v8::TryCatch *exc, bool to_shell=true)
  {
    v8::Handle<v8::Message> message = exc->Message();
    std::string exception_text = format_exception(v8_value_to_shcore_value(exc->Exception()));

    if (message.IsEmpty())
    {
      if (to_shell)
        delegate->print_error(delegate->user_data, std::string("JS Exception: ").append(exception_text).append("\n").c_str());
      else
        std::cerr << std::string("JS Exception: ").append(exception_text).append("\n");
    }
    else
    {
      v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());

      // location
      std::string text = (boost::format("JS Exception: %s:%i:%i: %s\n") % *filename % message->GetLineNumber() % message->GetStartColumn() % exception_text).str();
      if (to_shell)
        delegate->print_error(delegate->user_data, text.c_str());
      else
        std::cerr << text;

      // code
      v8::String::Utf8Value code(message->GetSourceLine());
      if (to_shell)
        delegate->print_error(delegate->user_data, (std::string(*code ? *code : "").append("\n")).c_str());
      else
        std::cerr << *code << "\n";

      v8::String::Utf8Value stack(exc->StackTrace());
      if (*stack && **stack)
      {
        if (to_shell)
          delegate->print_error(delegate->user_data, (std::string(*stack).append("\n")).c_str());
        else
          std::cerr << *stack << "\n";
      }
    }
  }

  Value v8_value_to_shcore_value(const v8::Handle<v8::Value> &value)
  {
    if (value->IsUndefined())
    {
      return Value();
    }
    if (value->IsNull())
      return Value::Null();
    else if (value->IsInt32())
      return Value((int64_t)value->ToInt32()->Value());
    else if (value->IsNumber())
      return Value(value->ToNumber()->Value());
    else if (value->IsString())
    {
      v8::String::Utf8Value s(value->ToString());
      return Value(std::string(*s, s.length()));
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
      throw std::logic_error("JS function wrapping not implemented");
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
      throw std::invalid_argument("Cannot convert JS value to internal value: "+std::string(*s));
    }
    return Value();
  }


  v8::Handle<v8::Value> shcore_value_to_v8_value(const Value &value)
  {
    v8::Handle<v8::Value> r;
    switch (value.type)
    {
    case Undefined:
      break;
    case Null:
      r = v8::Null(isolate);
      break;
    case Bool:
      r = v8::Boolean::New(isolate, value.value.b);
      break;
    case String:
      r = v8::String::NewFromUtf8(isolate, value.value.s->c_str());
      break;
    case Integer:
      r = v8::Integer::New(isolate, value.value.i);
      break;
    case Float:
      r = v8::Number::New(isolate, value.value.d);
      break;
    case Object:
      r = wrap_object(*value.value.o);
      break;
    case Array:
      r = wrap_array(*value.value.array);
      break;
    case Map:
      r = wrap_map(*value.value.map);
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
      r= wrap_function(*value.value.func);
      break;
    }
    return r;
  }


  void set_global(const std::string &name, const v8::Handle<v8::Value> &value)
  {
    v8::Handle<v8::Context> ctx(v8::Local<v8::Context>::New(isolate, context));
    ctx->Global()->Set(v8::String::NewFromUtf8(isolate, name.c_str()), value);
  }

  v8::Handle<v8::Value> get_global(const std::string &name)
  {
    v8::Handle<v8::Context> ctx(v8::Local<v8::Context>::New(isolate, context));
    return ctx->Global()->Get(v8::String::NewFromUtf8(isolate, name.c_str()));
  }


  v8::Handle<v8::Value> wrap_object(const boost::shared_ptr<Object_bridge> &o)
  {
    if (!object_wrapper)
      object_wrapper = new JScript_object_wrapper(owner);
    return object_wrapper->wrap(o);
  }

  v8::Handle<v8::Value> wrap_map(const boost::shared_ptr<Value::Map_type> &o)
  {
    if (!map_wrapper)
      map_wrapper = new JScript_map_wrapper(owner);
    return map_wrapper->wrap(o);
  }

  v8::Handle<v8::Value> wrap_array(const boost::shared_ptr<Value::Array_type> &o)
  {
    if (!array_wrapper)
      array_wrapper = new JScript_array_wrapper(owner);
    return array_wrapper->wrap(o);
  }

  v8::Handle<v8::Value> wrap_function(const boost::shared_ptr<Function_base> &f)
  {
    if (!function_wrapper)
      function_wrapper = new JScript_function_wrapper(owner);
    return function_wrapper->wrap(f);
  }
};


/** Initializer for JS stuff

 Must be called once when the program is started.
 */
void JScript_context_init()
{
  static bool inited = false;
  if (!inited)
  {
//    InitializeICU();
//    Platform* platform = platform::CreateDefaultPlatform();
//    InitializePlatform(platform);
    v8::V8::Initialize();
  }
}


JScript_context::JScript_context(Object_registry *registry, Interpreter_delegate *deleg)
: _impl(new JScript_context_impl(this, deleg)), _registry(registry)
{
  set_global("_G", Value(registry->_registry));
}


JScript_context::~JScript_context()
{
  delete _impl;
}


void JScript_context::set_global(const std::string &name, const Value &value)
{
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch;
  // set _context to be the default context for everything in this scope
  v8::Context::Scope context_scope(v8::Local<v8::Context>::New(_impl->isolate, _impl->context));

  _impl->set_global(name, _impl->shcore_value_to_v8_value(value));
}


Value JScript_context::get_global(const std::string &name)
{
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch;
  // set _context to be the default context for everything in this scope
  v8::Context::Scope context_scope(v8::Local<v8::Context>::New(_impl->isolate, _impl->context));

  return _impl->v8_value_to_shcore_value(_impl->get_global(name));
}


v8::Isolate *JScript_context::isolate() const
{
  return _impl->isolate;
}


v8::Handle<v8::Context> JScript_context::context() const
{
  return v8::Local<v8::Context>::New(isolate(), _impl->context);
}


Value JScript_context::v8_value_to_shcore_value(const v8::Handle<v8::Value> &value)
{
  return _impl->v8_value_to_shcore_value(value);
}


v8::Handle<v8::Value> JScript_context::shcore_value_to_v8_value(const Value &value)
{
  return _impl->shcore_value_to_v8_value(value);
}


Argument_list JScript_context::convert_args(const v8::FunctionCallbackInfo<v8::Value>& args)
{
  return _impl->convert_args(args);
}



Value JScript_context::execute(const std::string &code_str, boost::system::error_code &ret_error) throw (Exception)
{
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch;
  // set _context to be the default context for everything in this scope
  v8::Context::Scope context_scope(v8::Local<v8::Context>::New(_impl->isolate, _impl->context));
  v8::ScriptOrigin origin(v8::String::NewFromUtf8(_impl->isolate, "(shell)"));
  v8::Handle<v8::String> code = v8::String::NewFromUtf8(_impl->isolate, code_str.c_str());
  v8::Handle<v8::Script> script = v8::Script::Compile(code, &origin);

  if (script.IsEmpty())
  {
    Value e(_impl->v8_value_to_shcore_value(try_catch.Exception()));
    _impl->print_exception(&try_catch, false);
    // TODO: wrap the Exception object in JS so that one can throw common exceptions from JS
    if (e.type == Map)
      throw Exception(e.as_map());
    else
      throw Exception::scripting_error(_impl->format_exception(e));
  }
  else
  {
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty())
    {
      Value e(_impl->v8_value_to_shcore_value(try_catch.Exception()));
      _impl->print_exception(&try_catch, false);
      if (e.type == Map)
        throw Exception(e.as_map());
      else
        throw Exception::scripting_error(_impl->format_exception(e));
    }
    else
    {
#if 0
      if (!result->IsUndefined())
      {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        v8::String::Utf8Value str(result);
        std::cout << ">>"<<*str << "\n";
      }
#endif
      return v8_value_to_shcore_value(result);
    }
  }
}


bool JScript_context::execute_interactive(const std::string &code_str) BOOST_NOEXCEPT_OR_NOTHROW
{
  boost::system::error_code error;
  Value result = execute(code_str, error);
  if (!error)
  {
    if (result && result.type != Null)
      _impl->delegate->print(_impl->delegate->user_data, result.descr(true).c_str());
    return true;
  }
  else
  {
    _impl->delegate->print_error(_impl->delegate->user_data, error.message().c_str());
  }
  return false;
}


int JScript_context::run_script(const std::string &path, boost::system::error_code &err) BOOST_NOEXCEPT_OR_NOTHROW
{
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch;
  // set _context to be the default context for everything in this scope
  v8::Context::Scope context_scope(v8::Local<v8::Context>::New(_impl->isolate, _impl->context));
  v8::ScriptOrigin origin(v8::String::NewFromUtf8(_impl->isolate, path.c_str()));
  v8::Handle<v8::Script> script;
  {
    std::ifstream s(path.c_str());
    if (s.fail())
    {
      err = errc::make_error_code((errc::errc_t)errno);
      return 1;
    }
    s.seekg(0, std::ios_base::end);
    std::streamsize fsize = s.tellg();
    s.seekg(0, std::ios_base::beg);
    char *fdata = new char[fsize+1];
    s.read(fdata, fsize);
    fdata[fsize] = 0;
    script = v8::Script::Compile(v8::String::NewFromUtf8(_impl->isolate, fdata), &origin);
    delete []fdata;

    if (script.IsEmpty())
    {
      _impl->print_exception(&try_catch, false);
      // XXX set the error to a proper value
      return 1;
    }
  }

  v8::Handle<v8::Value> result = script->Run();
  if (result.IsEmpty())
  {
    _impl->print_exception(&try_catch, false);
    // XXX set the error to a proper value
    return 1;
  }
  else
  {
    return 0;
  }
}


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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "shellcore/jscript_context.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "mysh_config.h"

#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"
#include "shellcore/object_factory.h"
#include "shellcore/object_registry.h"

#include "shellcore/jscript_object_wrapper.h"
#include "shellcore/jscript_function_wrapper.h"
#include "shellcore/jscript_map_wrapper.h"
#include "shellcore/jscript_array_wrapper.h"
#include "shellcore/shell_registry.h"

#include "shellcore/jscript_type_conversion.h"
#include "shellcore/jscript_core_definitions.h"
#include <boost/weak_ptr.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <cerrno>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif
#include "utils_file.h"

using namespace shcore;
using namespace boost::system;

struct JScript_context::JScript_context_impl
{
  JScript_context *owner;
  JScript_type_bridger types;

  v8::Isolate *isolate;
  v8::Persistent<v8::Context> context;
  std::map<std::string, v8::Persistent<v8::Object>* > factory_packages;
  v8::Persistent<v8::ObjectTemplate> package_template;

  Interpreter_delegate *delegate;

  JScript_context_impl(JScript_context *owner_, Interpreter_delegate *deleg)
    : owner(owner_), types(owner_), isolate(v8::Isolate::New()), delegate(deleg)
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::ObjectTemplate> globals = v8::ObjectTemplate::New(isolate);
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

    globals->Set(v8::String::NewFromUtf8(isolate, "os"), make_os_object());

    globals->Set(v8::String::NewFromUtf8(isolate, "shell"), make_shell_object());

    // obj = _F.mysql.open('root@localhost')
    globals->Set(v8::String::NewFromUtf8(isolate, "_F"), make_factory());

    {
      v8::Handle<v8::ObjectTemplate> templ(v8::ObjectTemplate::New(isolate));
      package_template.Reset(isolate, templ);
      templ->SetNamedPropertyHandler(&JScript_context_impl::factory_package_getter, 0, 0, 0, 0, client_data);
    }

    // source('module')
    globals->Set(v8::String::NewFromUtf8(isolate, "source"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_source, client_data));

    globals->Set(v8::String::NewFromUtf8(isolate, "__build_module"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_build_module, client_data));

    v8::Local<v8::Context> lcontext = v8::Context::New(isolate, NULL, globals);
    context.Reset(isolate, lcontext);

    // Loads the core module
    load_core_module();
  }

  /*
  * load_core_module loads the content of the given module file
  * and inserts the definitions on the JS globals.
  */
  void load_core_module()
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Value> result;
    //std::string source, error;

    std::string source = "(function (){" + shcore::js_core_module + "});";

    // Result must be valid or an exception is thrown.
    result = _build_module(v8::String::NewFromUtf8(isolate, "core.js"), v8::String::NewFromUtf8(isolate, source.c_str()));

    // It is expected to have a function on the core module.
    if (result->IsFunction())
    {
      v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(result);
      v8::Local<v8::Context> lcontext = v8::Local<v8::Context>::New(isolate, context);

      f->Call(lcontext->Global(), 0, NULL);
    }
    else
      // If this happens is because the core.js was messed up!
      throw shcore::Exception::runtime_error("Unable to load the core module!!");
  }

  ~JScript_context_impl()
  {
    for (std::map<std::string, v8::Persistent<v8::Object>* >::iterator i = factory_packages.begin(); i != factory_packages.end(); ++i)
      delete i->second;

    types.dispose();

    // Releases the context
    context.Reset();
    isolate->Dispose();
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
        info.GetIsolate()->ThrowException(v8::String::NewFromUtf8(info.GetIsolate(), ("Invalid package " + pkgname).c_str()));
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

  v8::Handle<v8::ObjectTemplate> make_shell_object()
  {
    v8::Handle<v8::ObjectTemplate> object = v8::ObjectTemplate::New(isolate);
    v8::Local<v8::External> client_data(v8::External::New(isolate, this));

    // s = input('Type something:')
    object->Set(v8::String::NewFromUtf8(isolate, "prompt"),
                 v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_prompt, client_data));

    return object;
  }

  // TODO: Creation of the OS module should be moved to a different place.
  v8::Handle<v8::ObjectTemplate> make_os_object()
  {
    v8::Handle<v8::ObjectTemplate> object = v8::ObjectTemplate::New(isolate);
    v8::Local<v8::External> client_data(v8::External::New(isolate, this));

    // repr(object) -> string
    object->Set(v8::String::NewFromUtf8(isolate, "getenv"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::os_getenv, client_data));

    object->Set(v8::String::NewFromUtf8(isolate, "get_user_config_path"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::os_get_user_config_path, client_data));

    object->Set(v8::String::NewFromUtf8(isolate, "get_mysqlx_home_path"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::os_get_mysqlx_home_path, client_data));

    object->Set(v8::String::NewFromUtf8(isolate, "get_binary_folder"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::os_get_binary_folder, client_data));

    object->Set(v8::String::NewFromUtf8(isolate, "file_exists"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::os_file_exists, client_data));

    object->Set(v8::String::NewFromUtf8(isolate, "load_text_file"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::os_load_text_file, client_data));

    object->Set(v8::String::NewFromUtf8(isolate, "sleep"),
      v8::FunctionTemplate::New(isolate, &JScript_context_impl::os_sleep, client_data));

    return object;
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
        args.GetReturnValue().Set(self->types.shcore_value_to_v8_value(result));
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
          self->types.v8_value_to_shcore_value(args[0]).repr().c_str()));
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
        args.GetReturnValue().Set(self->types.shcore_value_to_v8_value(Value::parse(std::string(*s))));
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
        args.GetReturnValue().Set(self->types.type_info(args[0]));
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
        std::string format = (*Shell_core_options::get())[SHCORE_OUTPUT_FORMAT].as_string();
        if (format.find("json") == 0)
          self->delegate->print(self->delegate->user_data, self->types.v8_value_to_shcore_value(args[i]).json(format == "json").c_str());
        else
          self->delegate->print(self->delegate->user_data, self->types.v8_value_to_shcore_value(args[i]).descr(true).c_str());
      }
      catch (std::exception &e)
      {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), e.what()));
        break;
      }
    }
  }

  static void f_prompt(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope outer_handle_scope(args.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    shcore::Value::Map_type_ref options_map;

    if (args.Length() < 1 || args.Length() > 2)
    {
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "Invalid number of parameters"));
      return;
    }
    else if (args.Length() == 2)
    {
      shcore::Value options = self->types.v8_value_to_shcore_value(args[1]);
      if (options.type != shcore::Map)
      {
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "Second parameter must be a dictionary"));
        return;
      }
      else
        options_map = options.as_map();
    }

    v8::HandleScope handle_scope(args.GetIsolate());
    v8::String::Utf8Value str(args[0]);

    // If there are options, reads them to determine how to proceed
    std::string default_value;
    bool password = false;
    bool succeeded = false;

    // Identifies a default value in case en empty string is returned
    // or hte prompt fails
    if (options_map)
    {
      if (options_map->has_key("defaultValue"))
        default_value = options_map->get_string("defaultValue");

      // Identifies if it is normal prompt or password prompt
      if (options_map->has_key("type"))
        password = (options_map->get_string("type") == "password");
    }

    // Performs the actual prompt
    std::string r;
    if (password)
      succeeded = self->delegate->password(self->delegate->user_data, *str, r);
    else
      succeeded = self->delegate->prompt(self->delegate->user_data, *str, r);

    // Uses the default value if needed
    if (!default_value.empty() && (!succeeded || r.empty()))
      r = default_value;

    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), r.c_str()));
  }

  static void f_source(const v8::FunctionCallbackInfo<v8::Value>& args)
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
    self->delegate->source(self->delegate->user_data, *str);
  }

  v8::Local<v8::Value> _build_module(v8::Handle<v8::String> origin, v8::Handle<v8::String> source)
  {
    v8::Local<v8::Value> result;
    // makes _isolate the default isolate for this context
    v8::EscapableHandleScope handle_scope(isolate);

    v8::TryCatch try_catch;
    // set _context to be the default context for everything in this scope
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(isolate, context));

    v8::Local<v8::Script> script;
    script = v8::Script::Compile(source, origin);
    if (!script.IsEmpty())
      result = script->Run();

    if (result.IsEmpty())
    {
      v8::String::Utf8Value exec_error(try_catch.Exception());
      std::string exception_text = "Error loading module at " + std::string(*v8::String::Utf8Value(origin)) + ". ";
      exception_text.append(*exec_error);

      throw shcore::Exception::scripting_error(exception_text);
    }

    return handle_scope.Escape(result);
  }

  static void f_build_module(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    v8::Local<v8::String> origin = v8::Local<v8::String>::Cast(args[0]);
    v8::Local<v8::String> source = v8::Local<v8::String>::Cast(args[1]);

    // Build the module which will return the built function
    v8::Local<v8::Value> result = self->_build_module(origin, source);

    args.GetReturnValue().Set(result);
  }

  // --------------------
  Argument_list convert_args(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    Argument_list r;

    for (int c = args.Length(), i = 0; i < c; i++)
    {
      r.push_back(types.v8_value_to_shcore_value(args[i]));
    }
    return r;
  }

  void print_exception(const std::string& text)
  {
    delegate->print_error(delegate->user_data, text.c_str());
  }

  void set_global_item(const std::string &global_name, const std::string &item_name, const v8::Handle<v8::Value> &value)
  {
    v8::Handle<v8::Value> global = get_global(global_name);

    v8::Handle<v8::Object> object = v8::Handle<v8::Object>::Cast(global);

    object->Set(v8::String::NewFromUtf8(isolate, item_name.c_str()), value);
  }

  void set_global(const std::string &name, const v8::Handle<v8::Value> &value)
  {
    v8::Handle<v8::Context> ctx(v8::Local<v8::Context>::New(isolate, context));
    if (value.IsEmpty() || !*value)
      ctx->Global()->Set(v8::String::NewFromUtf8(isolate, name.c_str()), v8::Null(isolate));
    else
      ctx->Global()->Set(v8::String::NewFromUtf8(isolate, name.c_str()), value);
  }

  v8::Handle<v8::Value> get_global(const std::string &name)
  {
    v8::Handle<v8::Context> ctx(v8::Local<v8::Context>::New(isolate, context));
    return ctx->Global()->Get(v8::String::NewFromUtf8(isolate, name.c_str()));
  }

  static void os_sleep(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());

    if (args.Length() != 1 || !args[0]->IsNumber())
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "sleep(<number>) takes 1 numeric argument"));
    else
    {
#ifdef HAVE_SLEEP
      sleep(args[0]->ToNumber()->Value());
#elif defined(WIN32)
      Sleep((DWORD)(args[0]->ToNumber()->Value() * 1000));
#endif
      args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
    }
  }

  static void os_getenv(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());

    if (args.Length() != 1)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "getenv(<name>) takes 1 argument"));
    else
    {
      v8::String::Utf8Value variable_name(args[0]);
      char *value = getenv(*variable_name);

      if (value)
        args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), value));
      else
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
    }
  }

  static void os_file_exists(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());

    if (args.Length() != 1)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "file_exists(<path>) takes 1 argument"));
    else
    {
      v8::String::Utf8Value path(args[0]);
      args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), file_exists(std::string(*path))));
    }
  }

  static void os_load_text_file(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());

    if (args.Length() != 1)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "load_text_file(<path>) takes 1 argument"));
    else
    {
      v8::String::Utf8Value path(args[0]);
      std::string source;

      if (load_text_file(*path, source))
      {
        args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), source.c_str()));
      }
      else
      {
        std::string error("load_text_file: unable to open file: ");
        error.append(*path);
        args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), error.c_str()));
      }
    }
  }

  static void os_get_user_config_path(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());

    if (args.Length() != 0)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "get_user_config_path() takes 0 arguments"));
    else
    {
      try
      {
        std::string path = get_user_config_path();
        args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), path.c_str()));
      }
      catch (...)
      {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
      }
    }
  }

  static void os_get_mysqlx_home_path(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());

    if (args.Length() != 0)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "get_mysqlx_home_path() takes 0 arguments"));
    else
    {
      try
      {
        std::string path = get_mysqlx_home_path();
        args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), path.c_str()));
      }
      catch (...)
      {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
      }
    }
  }

  static void os_get_binary_folder(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope handle_scope(args.GetIsolate());

    if (args.Length() != 0)
      args.GetIsolate()->ThrowException(v8::String::NewFromUtf8(args.GetIsolate(), "get_binary_folder() takes 0 arguments"));
    else
    {
      try
      {
        std::string path = get_binary_folder();
        args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), path.c_str()));
      }
      catch (...)
      {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
      }
    }
  }
};

/** Initializer for JS stuff

 Must be called once when the program is started.
 */
void SHCORE_PUBLIC JScript_context_init()
{
  static bool inited = false;
  if (!inited)
  {
    //    InitializeICU();
    //    Platform* platform = platform::CreateDefaultPlatform();
    //    InitializePlatform(platform);
    v8::V8::Initialize();
    inited = true;
  }
}

JScript_context::JScript_context(Object_registry *registry, Interpreter_delegate *deleg)
  : _impl(new JScript_context_impl(this, deleg)), _registry(registry)
{
  // initialize type conversion class now that everything is ready
  {
    v8::Isolate::Scope isolate_scope(_impl->isolate);
    v8::HandleScope handle_scope(_impl->isolate);
    v8::TryCatch try_catch;
    v8::Context::Scope context_scope(v8::Local<v8::Context>::New(_impl->isolate, _impl->context));
    _impl->types.init();
  }

  set_global("globals", Value(registry->_registry));
  set_global_item("shell", "options", Value(boost::static_pointer_cast<Object_bridge>(Shell_core_options::get_instance())));
  set_global_item("shell", "storedSessions", StoredSessions::get());
}

void JScript_context::set_global_item(const std::string& global_name, const std::string& item_name, const Value &value)
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

  _impl->set_global_item(global_name, item_name, _impl->types.shcore_value_to_v8_value(value));
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

  _impl->set_global(name, _impl->types.shcore_value_to_v8_value(value));
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

  return _impl->types.v8_value_to_shcore_value(_impl->get_global(name));
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
  return _impl->types.v8_value_to_shcore_value(value);
}

v8::Handle<v8::Value> JScript_context::shcore_value_to_v8_value(const Value &value)
{
  return _impl->types.shcore_value_to_v8_value(value);
}

Argument_list JScript_context::convert_args(const v8::FunctionCallbackInfo<v8::Value>& args)
{
  return _impl->convert_args(args);
}

Value JScript_context::execute(const std::string &code_str, const std::string& source) throw (Exception)
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
  v8::ScriptOrigin origin(v8::String::NewFromUtf8(_impl->isolate, source.c_str()));
  v8::Handle<v8::String> code = v8::String::NewFromUtf8(_impl->isolate, code_str.c_str());
  v8::Handle<v8::Script> script = v8::Script::Compile(code, &origin);

  // Since ret_val can't be used to check whether all was ok or not
  // Will use a boolean flag
  Value ret_val;
  bool executed_ok = false;

  if (!script.IsEmpty())
  {
    v8::Handle<v8::Value> result = script->Run();
    if (!result.IsEmpty())
    {
      ret_val = v8_value_to_shcore_value(result);
      executed_ok = true;
    }
  }

  if (!executed_ok)
  {
    if (try_catch.HasCaught())
    {
      Value e = get_v8_exception_data(&try_catch);

      throw Exception::scripting_error(format_exception(e));
    }
    else
      throw shcore::Exception::logic_error("Unexpected error processing script, no exception caught!");
  }

  return ret_val;
}

Value JScript_context::execute_interactive(const std::string &code_str, Interactive_input_state& r_state) BOOST_NOEXCEPT_OR_NOTHROW
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

  r_state = Input_ok;

  if (script.IsEmpty())
  {
    // check if this was an error of type
    // SyntaxError: Unexpected end of input
    // which we treat as a multiline mode trigger
    v8::String::Utf8Value message(try_catch.Exception());
    if (*message && strcmp(*message, "SyntaxError: Unexpected end of input") == 0)
      r_state = Input_continued_block;
    else
      _impl->print_exception(format_exception(get_v8_exception_data(&try_catch)));
  }
  else
  {
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty())
      _impl->print_exception(format_exception(get_v8_exception_data(&try_catch)));
    else
    {
      try
      {
        return Value(v8_value_to_shcore_value(result));
      }
      catch (std::exception &exc)
      {
        // we used to let the exception bubble up, but somehow, exceptions thrown from v8_value_to_shcore_value()
        // aren't being caught from main.cc, leading to a crash due to unhandled exception.. so we catch and print it here
        _impl->delegate->print_error(_impl->delegate->user_data, std::string("INTERNAL ERROR: ").append(exc.what()).c_str());
      }
    }
  }
  return Value();
}

std::string JScript_context::format_exception(const shcore::Value &exc)
{
  std::string error_message;

  if (exc.type == Map)
  {
    std::string type = exc.as_map()->get_string("type", "");
    std::string message = exc.as_map()->get_string("message", "");
    int64_t code = exc.as_map()->get_int("code", -1);
    std::string location = exc.as_map()->get_string("location", "");

    if (!message.empty())
    {
      if (!type.empty())
        error_message += type;

      if (code != -1)
        error_message += (boost::format(" (%1%)") % code).str();

      if (!error_message.empty())
        error_message += ": ";

      error_message += message;

      if (!location.empty())
        error_message += " at " + location;
    }
  }
  else
    error_message = "Unexpected format of exception object.";

  error_message += "\n";

  return error_message;
}

Value JScript_context::get_v8_exception_data(v8::TryCatch *exc)
{
  Value::Map_type_ref data;

  v8::String::Utf8Value exec_error(exc->Exception());
  if (*exec_error)
  {
    // JS errors produced by V8 most likely will fall on this branch
    data.reset(new Value::Map_type());
    (*data)["message"] = Value(*exec_error);
  }
  else
  {
    // Errors produced by C++ code we exposed to the JS engine will fall here
    data = _impl->types.v8_value_to_shcore_value(exc->Exception()).as_map();
  }

  v8::Handle<v8::Message> message = exc->Message();
  if (!message.IsEmpty())
  {
    // location
    v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
    std::string text = (boost::format("%s:%i:%i\n") % *filename % message->GetLineNumber() % message->GetStartColumn()).str();
    v8::String::Utf8Value code(message->GetSourceLine());
    text.append("in ");
    text.append(*code ? *code : "").append("\n");

    // underline
    text.append(3 + message->GetStartColumn(), ' ');
    text.append(message->GetEndColumn() - message->GetStartColumn(), '^');
    text.append("\n");

    v8::String::Utf8Value stack(exc->StackTrace());
    if (*stack && **stack)
    {
      std::string str_stack(*stack);

      auto new_lines = std::count(str_stack.begin(), str_stack.end(), '\n');

      if (new_lines > 1)
        text.append(std::string(*stack).append("\n"));
    }

    (*data)["location"] = Value(text);
  }

  return Value(data);
}
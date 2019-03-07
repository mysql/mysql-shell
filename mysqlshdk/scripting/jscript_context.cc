/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/jscript_context.h"

#if V8_MAJOR_VERSION > 6 || (V8_MAJOR_VERSION == 6 && V8_MINOR_VERSION > 7)
#error "v8::Platform uses deprecated code, remove these undefs when it's fixed"
#else
#undef V8_DEPRECATE_SOON
#define V8_DEPRECATE_SOON(message, declarator) declarator
#include <libplatform/libplatform.h>
#endif

#include "mysqlshdk/include/mysh_config.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "scripting/module_registry.h"
#include "scripting/object_factory.h"
#include "scripting/object_registry.h"

#include "scripting/jscript_array_wrapper.h"
#include "scripting/jscript_function_wrapper.h"
#include "scripting/jscript_map_wrapper.h"
#include "scripting/jscript_object_wrapper.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

#include "scripting/jscript_core_definitions.h"
#include "scripting/jscript_type_conversion.h"

#include "mysqlshdk/include/shellcore/base_shell.h"  // FIXME

#include <cerrno>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif
#include "utils/utils_file.h"

namespace shcore {

namespace {

std::unique_ptr<v8::Platform> g_platform;

}  // namespace

/** Initializer for JS stuff
 *
 * Must be called once when the program is started.
 */
void SHCORE_PUBLIC JScript_context_init() {
  if (!g_platform) {
    g_platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(g_platform.get());
    v8::V8::Initialize();
  }
}

void SHCORE_PUBLIC JScript_context_fini() {
  if (g_platform) {
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    g_platform.reset(nullptr);
  }
}

struct JScript_context::JScript_context_impl {
  JScript_context *owner;
  JScript_type_bridger types;

  v8::Isolate *isolate;
  v8::Persistent<v8::Context> context;
  std::map<std::string, v8::Global<v8::Object>> factory_packages;
  v8::Persistent<v8::ObjectTemplate> package_template;

 private:
  std::unique_ptr<v8::ArrayBuffer::Allocator> m_allocator;
  bool m_terminating = false;
  std::vector<v8::Global<v8::Context>> m_stored_contexts;

 public:
  JScript_context_impl(JScript_context *owner_)
      : owner(owner_),
        types(owner_),
        isolate(nullptr),
        m_allocator(v8::ArrayBuffer::Allocator::NewDefaultAllocator()) {
    JScript_context_init();

    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = m_allocator.get();

    isolate = v8::Isolate::New(params);
    isolate->SetData(0, this);

    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::ObjectTemplate> globals = v8::ObjectTemplate::New(isolate);

    // register symbols to be exported to JS in global namespace

    // repr(object) -> string
    globals->Set(v8_string("repr"),
                 wrap_callback(&JScript_context_impl::f_repr));

    // unrepr(string) -> object
    globals->Set(v8_string("unrepr"),
                 wrap_callback(&JScript_context_impl::f_unrepr));

    // type(object) -> string
    globals->Set(v8_string("type"),
                 wrap_callback(&JScript_context_impl::f_type));

    // print('hello')
    globals->Set(
        v8_string("print"),
        wrap_callback([](const v8::FunctionCallbackInfo<v8::Value> &args) {
          f_print(args, false);
        }));

    globals->Set(
        v8_string("println"),
        wrap_callback([](const v8::FunctionCallbackInfo<v8::Value> &args) {
          f_print(args, true);
        }));

    globals->Set(v8_string("os"), make_os_object());

    // obj = _F.mysql.open('root@localhost')
    globals->Set(v8_string("_F"), make_factory());

    {
      v8::Local<v8::ObjectTemplate> templ(v8::ObjectTemplate::New(isolate));
      package_template.Reset(isolate, templ);

      v8::NamedPropertyHandlerConfiguration config;
      config.getter = &JScript_context_impl::factory_package_getter;
      config.flags = v8::PropertyHandlerFlags::kOnlyInterceptStrings;
      templ->SetHandler(config);
    }

    // source('module')
    globals->Set(v8_string("source"),
                 wrap_callback(&JScript_context_impl::f_source));

    globals->Set(v8_string("__require"),
                 wrap_callback(&JScript_context_impl::f_require));

    globals->Set(v8_string("__build_module"),
                 wrap_callback(&JScript_context_impl::f_build_module));

    v8::Local<v8::Context> lcontext = v8::Context::New(isolate, NULL, globals);
    context.Reset(isolate, lcontext);

    // Loads the core module
    load_core_module();
  }

  /*
   * load_core_module loads the content of the given module file
   * and inserts the definitions on the JS globals.
   */
  void load_core_module() {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Value> result;
    // std::string source, error;

    std::string source = "(function (){" + shcore::js_core_module + "});";

    // Result must be valid or an exception is thrown.
    result = _build_module(v8_string("core.js"), v8_string(source));

    // It is expected to have a function on the core module.
    if (result->IsFunction()) {
      v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(result);
      v8::Local<v8::Context> lcontext =
          v8::Local<v8::Context>::New(isolate, context);

      // call ToLocalChecked() to ensure we have a valid result
      f->Call(lcontext, lcontext->Global(), 0, nullptr).ToLocalChecked();
    } else
      // If this happens is because the core.js was messed up!
      throw shcore::Exception::runtime_error(
          "Unable to load the core module!!");
  }

  ~JScript_context_impl() {
    factory_packages.clear();
    types.dispose();

    {
      v8::HandleScope handle_scope(isolate);

      for (const auto &c : m_stored_contexts) {
        delete_context(c.Get(isolate));
      }

      m_stored_contexts.clear();
    }

    // release the context
    context.Reset();
    // notify the isolate
    isolate->ContextDisposedNotification();
    // force GC
    isolate->LowMemoryNotification();
    // dispose isolate
    isolate->Dispose();
  }

  // Factory interface implementation

  static void factory_getter(v8::Local<v8::Name> property,
                             const v8::PropertyCallbackInfo<v8::Value> &info) {
    const auto isolate = info.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    v8::HandleScope outer_handle_scope(isolate);
    const auto pkgname = to_string(isolate, property);
    const auto iter = self->factory_packages.find(pkgname);

    if (iter == self->factory_packages.end()) {
      // check if the package exists
      if (Object_factory::has_package(pkgname)) {
        auto package = self->make_factory_package(pkgname);
        info.GetReturnValue().Set(
            self->factory_packages
                .emplace(std::piecewise_construct,
                         std::forward_as_tuple(pkgname),
                         std::forward_as_tuple(self->isolate, package))
                .first->second);
      } else {
        isolate->ThrowException(
            v8_string(isolate, "Invalid package " + pkgname));
      }
    } else {
      info.GetReturnValue().Set(iter->second);
    }
  }

  v8::Local<v8::ObjectTemplate> make_factory() {
    v8::Local<v8::ObjectTemplate> factory = v8::ObjectTemplate::New(isolate);

    v8::NamedPropertyHandlerConfiguration config;
    config.getter = &JScript_context_impl::factory_getter;
    config.flags = v8::PropertyHandlerFlags::kOnlyInterceptStrings;
    factory->SetHandler(config);

    return factory;
  }

  // TODO: Creation of the OS module should be moved to a different place.
  v8::Local<v8::ObjectTemplate> make_os_object() {
    v8::Local<v8::ObjectTemplate> object = v8::ObjectTemplate::New(isolate);

    object->Set(v8_string("getenv"),
                wrap_callback(&JScript_context_impl::os_getenv));

    object->Set(v8_string("get_user_config_path"),
                wrap_callback(&JScript_context_impl::os_get_user_config_path));

    object->Set(v8_string("get_mysqlx_home_path"),
                wrap_callback(&JScript_context_impl::os_get_mysqlx_home_path));

    object->Set(v8_string("get_binary_folder"),
                wrap_callback(&JScript_context_impl::os_get_binary_folder));

    object->Set(v8_string("file_exists"),
                wrap_callback(&JScript_context_impl::os_file_exists));

    object->Set(v8_string("load_text_file"),
                wrap_callback(&JScript_context_impl::os_load_text_file));

    object->Set(v8_string("sleep"),
                wrap_callback(&JScript_context_impl::os_sleep));

    return object;
  }

  static void factory_package_getter(
      v8::Local<v8::Name> property,
      const v8::PropertyCallbackInfo<v8::Value> &info) {
    const auto isolate = info.GetIsolate();
    v8::HandleScope outer_handle_scope(isolate);

    const auto propname = to_string(isolate, property);

    if (strcmp(propname.c_str(), "__name__") != 0) {
      const auto self =
          static_cast<JScript_context_impl *>(isolate->GetData(0));
      auto maybe_name = info.This()->Get(isolate->GetCurrentContext(),
                                         v8_string(isolate, "__name__"));
      if (!maybe_name.IsEmpty()) {
        info.GetReturnValue().Set(self->wrap_factory_constructor(
            to_string(isolate, maybe_name.ToLocalChecked()), propname));
      }
    }
  }

  v8::Local<v8::Object> make_factory_package(const std::string &package) {
    const auto templ =
        v8::Local<v8::ObjectTemplate>::New(isolate, package_template);
    const auto lcontext = v8::Local<v8::Context>::New(isolate, context);
    const auto pkg = templ->NewInstance(lcontext).ToLocalChecked();
    pkg->Set(lcontext, v8_string("__name__"), v8_string(package)).FromJust();
    return pkg;
  }

  static void call_factory(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    if (self->is_terminating()) return;

    const auto lcontext = isolate->GetCurrentContext();
    const auto client_data = args.Data()->ToObject(lcontext).ToLocalChecked();

    const auto package =
        self->to_string(client_data->Get(lcontext, self->v8_string("package"))
                            .ToLocalChecked());
    const auto factory =
        self->to_string(client_data->Get(lcontext, self->v8_string("function"))
                            .ToLocalChecked());

    try {
      Value result(Object_factory::call_constructor(
          package, factory, self->convert_args(args),
          NamingStyle::LowerCamelCase));
      if (result)
        args.GetReturnValue().Set(self->types.shcore_value_to_v8_value(result));
    } catch (std::exception &e) {
      isolate->ThrowException(v8_string(isolate, e.what()));
    }
  }

  v8::Local<v8::Function> wrap_factory_constructor(const std::string &package,
                                                   const std::string &factory) {
    v8::Local<v8::Object> client_data(v8::Object::New(isolate));
    const auto lcontext = v8::Local<v8::Context>::New(isolate, context);

    client_data->Set(lcontext, v8_string("package"), v8_string(package))
        .FromJust();
    client_data->Set(lcontext, v8_string("function"), v8_string(factory))
        .FromJust();

    v8::Local<v8::FunctionTemplate> ftempl = v8::FunctionTemplate::New(
        isolate, &JScript_context_impl::call_factory, client_data);
    return ftempl->GetFunction(lcontext).ToLocalChecked();
  }

  // Global functions exposed to JS

  static void f_repr(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 1) {
      isolate->ThrowException(v8_string(isolate, "repr() takes 1 argument"));
    } else {
      try {
        args.GetReturnValue().Set(v8_string(
            isolate, self->types.v8_value_to_shcore_value(args[0]).repr()));
      } catch (std::exception &e) {
        isolate->ThrowException(v8_string(isolate, e.what()));
      }
    }
  }

  static void f_unrepr(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 1) {
      isolate->ThrowException(v8_string(isolate, "unrepr() takes 1 argument"));
    } else {
      try {
        args.GetReturnValue().Set(self->types.shcore_value_to_v8_value(
            Value::parse(to_string(isolate, args[0]))));
      } catch (std::exception &e) {
        isolate->ThrowException(v8_string(isolate, e.what()));
      }
    }
  }

  static void f_type(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 1) {
      isolate->ThrowException(v8_string(isolate, "type() takes 1 argument"));
    } else {
      try {
        args.GetReturnValue().Set(self->types.type_info(args[0]));
      } catch (std::exception &e) {
        isolate->ThrowException(v8_string(isolate, e.what()));
      }
    }
  }

  static void f_print(const v8::FunctionCallbackInfo<v8::Value> &args,
                      bool new_line) {
    const auto isolate = args.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    v8::HandleScope outer_handle_scope(isolate);

    std::string text;
    // FIXME this doesn't belong here?
    std::string format = mysqlsh::current_shell_options()->get().wrap_json;

    for (int i = 0; i < args.Length(); i++) {
      v8::HandleScope handle_scope(isolate);
      if (i > 0) text.push_back(' ');

      try {
        if (format.find("json") == 0)
          text += self->types.v8_value_to_shcore_value(args[i]).json(format ==
                                                                     "json");
        else
          text += self->types.v8_value_to_shcore_value(args[i]).descr(true);
      } catch (std::exception &e) {
        isolate->ThrowException(v8_string(isolate, e.what()));
        break;
      }
    }
    if (new_line) text.append("\n");

    mysqlsh::current_console()->print(text);
  }

  static void f_source(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    v8::HandleScope outer_handle_scope(isolate);

    if (args.Length() != 1) {
      isolate->ThrowException(
          v8_string(isolate, "Invalid number of parameters"));
      return;
    }

    v8::HandleScope handle_scope(isolate);
    const auto str = to_string(isolate, args[0]);

    // Loads the source content
    std::string source;
    if (load_text_file(str, source)) {
      self->owner->execute(source, str, {});
    } else {
      isolate->ThrowException(v8_string(isolate, "Error loading script"));
    }
  }

  static void f_require(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 1) {
      isolate->ThrowException(
          v8_string(isolate, "Invalid number of parameters"));
    } else {
      try {
        const auto s = to_string(isolate, args[0]);

        auto core_modules = Object_factory::package_contents("__modules__");
        if (std::find(core_modules.begin(), core_modules.end(), s) !=
            core_modules.end()) {
          auto module = Object_factory::call_constructor(
              "__modules__", s, shcore::Argument_list(),
              NamingStyle::LowerCamelCase);
          args.GetReturnValue().Set(self->types.shcore_value_to_v8_value(
              shcore::Value(std::dynamic_pointer_cast<Object_bridge>(module))));
        }
      } catch (std::exception &e) {
        isolate->ThrowException(v8_string(isolate, e.what()));
      }
    }
  }

  v8::Local<v8::Value> _build_module(v8::Local<v8::String> origin,
                                     v8::Local<v8::String> source) {
    v8::MaybeLocal<v8::Value> result;
    // makes _isolate the default isolate for this context
    v8::EscapableHandleScope handle_scope(isolate);

    v8::TryCatch try_catch{isolate};
    // set _context to be the default context for everything in this scope
    v8::Local<v8::Context> lcontext =
        v8::Local<v8::Context>::New(isolate, context);
    v8::Context::Scope context_scope(lcontext);

    v8::MaybeLocal<v8::Script> script;
    v8::ScriptOrigin script_origin{origin};
    script = v8::Script::Compile(lcontext, source, &script_origin);
    if (!script.IsEmpty()) result = script.ToLocalChecked()->Run(lcontext);

    if (result.IsEmpty()) {
      std::string exception_text = "Error loading module at " +
                                   to_string(origin) + ". " +
                                   to_string(try_catch.Exception());

      throw shcore::Exception::scripting_error(exception_text);
    }

    return handle_scope.Escape(result.ToLocalChecked());
  }

  static void f_build_module(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();
    const auto self = static_cast<JScript_context_impl *>(isolate->GetData(0));

    v8::Local<v8::String> origin = v8::Local<v8::String>::Cast(args[0]);
    v8::Local<v8::String> source = v8::Local<v8::String>::Cast(args[1]);

    // Build the module which will return the built function
    v8::Local<v8::Value> result = self->_build_module(origin, source);

    args.GetReturnValue().Set(result);
  }

  // --------------------
  Argument_list convert_args(const v8::FunctionCallbackInfo<v8::Value> &args) {
    Argument_list r;

    for (int c = args.Length(), i = 0; i < c; i++) {
      r.push_back(types.v8_value_to_shcore_value(args[i]));
    }
    return r;
  }

  void print_exception(const std::string &text) {
    mysqlsh::current_console()->print_diag(text);
  }

  void set_global_item(const std::string &global_name,
                       const std::string &item_name,
                       const v8::Local<v8::Value> &value) {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Value> global = get_global(global_name);
    v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(global);
    const auto lcontext = v8::Local<v8::Context>::New(isolate, context);

    object->Set(lcontext, v8_string(item_name), value).FromJust();
  }

  void set_global(const std::string &name, const v8::Local<v8::Value> &value) {
    v8::HandleScope handle_scope(isolate);
    const auto ctx = v8::Local<v8::Context>::New(isolate, context);
    if (value.IsEmpty() || !*value)
      ctx->Global()->Set(ctx, v8_string(name), v8::Null(isolate)).FromJust();
    else
      ctx->Global()->Set(ctx, v8_string(name), value).FromJust();
  }

  v8::Local<v8::Value> get_global(const std::string &name) {
    const auto ctx = v8::Local<v8::Context>::New(isolate, context);
    return ctx->Global()->Get(ctx, v8_string(name)).ToLocalChecked();
  }

  static void os_sleep(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 1 || !args[0]->IsNumber()) {
      isolate->ThrowException(
          v8_string(isolate, "sleep(<number>) takes 1 numeric argument"));
    } else {
      shcore::sleep_ms(args[0]
                           ->ToNumber(isolate->GetCurrentContext())
                           .ToLocalChecked()
                           ->Value() *
                       1000.0);
    }
  }

  static void os_getenv(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 1) {
      isolate->ThrowException(
          v8_string(isolate, "getenv(<name>) takes 1 argument"));
    } else {
      const auto variable_name = to_string(isolate, args[0]);
      char *value = getenv(variable_name.c_str());

      if (value)
        args.GetReturnValue().Set(v8_string(isolate, value));
      else
        args.GetReturnValue().Set(v8::Null(isolate));
    }
  }

  static void os_file_exists(const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 1) {
      isolate->ThrowException(
          v8_string(isolate, "file_exists(<path>) takes 1 argument"));
    } else {
      args.GetReturnValue().Set(
          v8::Boolean::New(isolate, is_file(to_string(isolate, args[0]))));
    }
  }

  static void os_load_text_file(
      const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 1) {
      isolate->ThrowException(
          v8_string(isolate, "load_text_file(<path>) takes 1 argument"));
    } else {
      const auto path = to_string(isolate, args[0]);
      std::string source;

      if (load_text_file(path, source)) {
        args.GetReturnValue().Set(v8_string(isolate, source));
      } else {
        isolate->ThrowException(
            v8_string(isolate, "load_text_file: unable to open file: " + path));
      }
    }
  }

  static void os_get_user_config_path(
      const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 0) {
      isolate->ThrowException(
          v8_string(isolate, "get_user_config_path() takes 0 arguments"));
    } else {
      try {
        std::string path = get_user_config_path();
        args.GetReturnValue().Set(v8_string(isolate, path));
      } catch (...) {
        args.GetReturnValue().Set(v8::Null(isolate));
      }
    }
  }

  static void os_get_mysqlx_home_path(
      const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 0) {
      isolate->ThrowException(
          v8_string(isolate, "get_mysqlx_home_path() takes 0 arguments"));
    } else {
      try {
        std::string path = get_mysqlx_home_path();
        args.GetReturnValue().Set(v8_string(isolate, path));
      } catch (...) {
        args.GetReturnValue().Set(v8::Null(isolate));
      }
    }
  }

  static void os_get_binary_folder(
      const v8::FunctionCallbackInfo<v8::Value> &args) {
    const auto isolate = args.GetIsolate();

    v8::HandleScope handle_scope(isolate);

    if (args.Length() != 0) {
      isolate->ThrowException(
          v8_string(isolate, "get_binary_folder() takes 0 arguments"));
    } else {
      try {
        std::string path = get_binary_folder();
        args.GetReturnValue().Set(v8_string(isolate, path));
      } catch (...) {
        args.GetReturnValue().Set(v8::Null(isolate));
      }
    }
  }

  static v8::Local<v8::String> v8_string(v8::Isolate *isolate,
                                         const char *data) {
    return shcore::v8_string(isolate, data);
  }

  static v8::Local<v8::String> v8_string(v8::Isolate *isolate,
                                         const std::string &data) {
    return v8_string(isolate, data.c_str());
  }

  v8::Local<v8::String> v8_string(const char *data) {
    return v8_string(isolate, data);
  }

  v8::Local<v8::String> v8_string(const std::string &data) {
    return v8_string(data.c_str());
  }

  static std::string to_string(v8::Isolate *isolate, v8::Local<v8::Value> obj) {
    return shcore::to_string(isolate, obj);
  }

  std::string to_string(v8::Local<v8::Value> obj) const {
    return to_string(isolate, obj);
  }

  void terminate() {
    // TerminateExecution() generates an exception which cannot be caught in JS.
    // It can be called from any thread, which means that it does not
    // immediately raise an exception, instead it sets a flag which will be
    // checked by V8 engine at some in the future by
    // v8::internal::StackGuard::HandleInterrupts() and then generate an
    // exception. For some reason (optimization?), if JS code has several
    // consecutive native calls, the execution is not terminated until all of
    // them are called, i.e.:
    //   os.sleep(10); println('failed');
    // pressing CTRL-C during sleep (which calls terminate()) will still print
    // 'failed', while:
    //   function fail() { println('failed'); } os.sleep(10); fail();
    // in the same scenario will work fine. For this reason we need to manually
    // check the `m_terminating` flag before executing native code.
    m_terminating = true;
    isolate->TerminateExecution();
  }

  bool is_terminating() const { return m_terminating; }

  void clear_is_terminating() { m_terminating = false; }

  v8::Local<v8::FunctionTemplate> wrap_callback(v8::FunctionCallback callback) {
    const auto data =
        v8::External::New(isolate, reinterpret_cast<void *>(callback));

    return v8::FunctionTemplate::New(
        isolate,
        [](const v8::FunctionCallbackInfo<v8::Value> &args) {
          const auto isolate = args.GetIsolate();
          const auto self =
              static_cast<JScript_context_impl *>(isolate->GetData(0));

          if (self->is_terminating()) return;

          const auto callback = reinterpret_cast<v8::FunctionCallback>(
              v8::External::Cast(*args.Data())->Value());
          callback(args);
        },
        data);
  }

  /**
   * Copies the global context.
   *
   * Context needs to be either deleted immediately (i.e. in case of an error),
   * or stored till JS context exists (in order to preserve execution
   * environment).
   */
  v8::Local<v8::Context> copy_global_context() const {
    const auto new_context = v8::Context::New(isolate);

    // copy globals into the new context
    const auto origin = context.Get(isolate);

    const auto globals = origin->Global();
    const auto names = globals->GetPropertyNames(origin).ToLocalChecked();

    const auto new_globals = new_context->Global();

    for (uint32_t c = names->Length(), i = 0; i < c; ++i) {
      const auto name = names->Get(origin, i).ToLocalChecked();
      new_globals->Set(new_context, name,
                       globals->Get(origin, name).ToLocalChecked());
    }

    return new_context;
  }

  void delete_context(v8::Local<v8::Context> context) const {
    const auto globals = context->Global();
    const auto names = globals->GetPropertyNames(context).ToLocalChecked();

    for (uint32_t c = names->Length(), i = 0; i < c; ++i) {
      const auto name = names->Get(context, i).ToLocalChecked();

      if (globals->Delete(context, name).IsNothing()) {
        log_error("Failed to delete global JS: %s", to_string(name).c_str());
      }
    }
  }

  void store_context(v8::Local<v8::Context> context) {
    m_stored_contexts.emplace_back(isolate, context);
  }
};

JScript_context::JScript_context(Object_registry *registry)
    : _impl(new JScript_context_impl(this)) {
  // initialize type conversion class now that everything is ready
  {
    v8::Isolate::Scope isolate_scope(_impl->isolate);
    v8::HandleScope handle_scope(_impl->isolate);
    v8::TryCatch try_catch{_impl->isolate};
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(_impl->isolate, _impl->context));
    _impl->types.init();
  }

  set_global("globals", Value(registry->_registry));
}

void JScript_context::set_global_item(const std::string &global_name,
                                      const std::string &item_name,
                                      const Value &value) {
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch{_impl->isolate};
  // set _context to be the default context for everything in this scope
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(_impl->isolate, _impl->context));

  _impl->set_global_item(global_name, item_name,
                         _impl->types.shcore_value_to_v8_value(value));
}

JScript_context::~JScript_context() { delete _impl; }

void JScript_context::set_global(const std::string &name, const Value &value) {
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch{_impl->isolate};
  // set _context to be the default context for everything in this scope
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(_impl->isolate, _impl->context));

  _impl->set_global(name, _impl->types.shcore_value_to_v8_value(value));
}

Value JScript_context::get_global(const std::string &name) {
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch{_impl->isolate};
  // set _context to be the default context for everything in this scope
  v8::Context::Scope context_scope(
      v8::Local<v8::Context>::New(_impl->isolate, _impl->context));

  return _impl->types.v8_value_to_shcore_value(_impl->get_global(name));
}

std::tuple<JSObject, std::string> JScript_context::get_global_js(
    const std::string &name) {
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch{_impl->isolate};
  // set _context to be the default context for everything in this scope
  const auto lcontext = context();
  v8::Context::Scope context_scope(lcontext);

  v8::Local<v8::Value> global(_impl->get_global(name));
  if (global->IsObject()) {
    auto obj_type = _impl->to_string(_impl->types.type_info(global));
    if (shcore::str_beginswith(obj_type, "m.")) obj_type = obj_type.substr(2);
    return std::tuple<JSObject, std::string>(
        JSObject(_impl->isolate, global->ToObject(lcontext).ToLocalChecked()),
        obj_type);
  } else {
    return std::tuple<JSObject, std::string>(JSObject(), "");
  }
}

std::vector<std::pair<bool, std::string>> JScript_context::list_globals() {
  Value globals(
      execute(
          "Object.keys(this).map(function(x) {"
          " if (typeof this[x] == 'function') return '('+x; else return '.'+x;"
          "})",
          {})
          .first);
  assert(globals.type == shcore::Array);

  std::vector<std::pair<bool, std::string>> ret;
  for (shcore::Value g : *globals.as_array()) {
    std::string n = g.get_string();
    ret.push_back({n[0] == '(', n.substr(1)});
  }
  return ret;
}

std::tuple<bool, JSObject, std::string> JScript_context::get_member_of(
    const JSObject *obj, const std::string &name) {
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch{_impl->isolate};
  // set _context to be the default context for everything in this scope
  const auto lcontext = context();
  v8::Context::Scope context_scope(lcontext);

  v8::Local<v8::Object> lobj(v8::Local<v8::Object>::New(_impl->isolate, *obj));

  auto maybe_value = lobj->Get(lcontext, v8_string(name));
  if (!maybe_value.IsEmpty()) {
    const auto value = maybe_value.ToLocalChecked();

    auto type = _impl->to_string(_impl->types.type_info(value));
    if (shcore::str_beginswith(type, "m.")) {
      type = type.substr(2);
    }

    if (value->IsFunction()) {
      return std::make_tuple(true, JSObject(), type);
    } else if (value->IsObject()) {
      const auto object = value->ToObject(lcontext).ToLocalChecked();
      return std::make_tuple(object->IsCallable(),
                             JSObject(_impl->isolate, object), type);
    }
  }

  return std::make_tuple(false, JSObject(), "");
}

std::vector<std::pair<bool, std::string>> JScript_context::get_members_of(
    const JSObject *obj) {
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch{_impl->isolate};
  // set _context to be the default context for everything in this scope
  const auto lcontext = context();
  v8::Context::Scope context_scope(lcontext);

  v8::Local<v8::Object> lobj(v8::Local<v8::Object>::New(_impl->isolate, *obj));

  std::vector<std::pair<bool, std::string>> keys;
  if (*lobj) {
    v8::Local<v8::Array> props(
        lobj->GetPropertyNames(lcontext).ToLocalChecked());
    for (size_t i = 0; i < props->Length(); i++) {
      auto maybe_value =
          lobj->Get(lcontext, props->Get(lcontext, i).ToLocalChecked());
      bool is_func;
      if (!maybe_value.IsEmpty()) {
        const auto value = maybe_value.ToLocalChecked();
        if (value->IsFunction()) {
          is_func = true;
        } else if (value->IsObject() &&
                   value->ToObject(lcontext).ToLocalChecked()->IsCallable()) {
          is_func = true;
        } else {
          is_func = false;
        }
        keys.push_back(
            {is_func,
             _impl->to_string(props->Get(lcontext, i).ToLocalChecked())});
      }
    }
  }
  return keys;
}

v8::Isolate *JScript_context::isolate() const { return _impl->isolate; }

v8::Local<v8::Context> JScript_context::context() const {
  return v8::Local<v8::Context>::New(isolate(), _impl->context);
}

Value JScript_context::v8_value_to_shcore_value(
    const v8::Local<v8::Value> &value) {
  return _impl->types.v8_value_to_shcore_value(value);
}

v8::Local<v8::Value> JScript_context::shcore_value_to_v8_value(
    const Value &value) {
  return _impl->types.shcore_value_to_v8_value(value);
}

Argument_list JScript_context::convert_args(
    const v8::FunctionCallbackInfo<v8::Value> &args) {
  return _impl->convert_args(args);
}

std::pair<Value, bool> JScript_context::execute(
    const std::string &code_str, const std::string &source,
    const std::vector<std::string> &argv) {
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch{_impl->isolate};
  // set _context to be the default context for everything in this scope
  v8::Local<v8::Context> lcontext = context();
  v8::Context::Scope context_scope(lcontext);
  v8::ScriptOrigin origin(v8_string(source));
  v8::Local<v8::String> code = v8_string(code_str);
  v8::MaybeLocal<v8::Script> script =
      v8::Script::Compile(lcontext, code, &origin);

  // Since ret_val can't be used to check whether all was ok or not
  // Will use a boolean flag

  shcore::Value args_value = get_global("sys").as_object()->get_member("argv");
  auto args = args_value.as_array();
  args->clear();
  for (auto &arg : argv) {
    args->push_back(Value(arg));
  }

  _impl->clear_is_terminating();

  if (!script.IsEmpty()) {
    v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(lcontext);
    if (is_terminating() || try_catch.HasTerminated()) {
      _impl->isolate->CancelTerminateExecution();
      mysqlsh::current_console()->print_diag(
          "Script execution interrupted by user.");

      return {Value(), true};
    } else if (!try_catch.HasCaught()) {
      return {v8_value_to_shcore_value(result.ToLocalChecked()), false};
    } else {
      Value e = get_v8_exception_data(try_catch, false);

      throw Exception::scripting_error(format_exception(e));
    }
  } else {
    if (try_catch.HasCaught()) {
      Value e = get_v8_exception_data(try_catch, false);

      throw Exception::scripting_error(format_exception(e));
    } else {
      throw shcore::Exception::logic_error(
          "Unexpected error compiling script, no exception caught!");
    }
  }
}

std::pair<Value, bool> JScript_context::execute_interactive(
    const std::string &code_str, Input_state *r_state) noexcept {
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch{_impl->isolate};
  // set _context to be the default context for everything in this scope
  v8::Local<v8::Context> lcontext = context();
  v8::Context::Scope context_scope(lcontext);
  v8::ScriptOrigin origin(v8_string("(shell)"));
  v8::Local<v8::String> code = v8_string(code_str);
  v8::MaybeLocal<v8::Script> script =
      v8::Script::Compile(lcontext, code, &origin);

  _impl->clear_is_terminating();

  *r_state = Input_state::Ok;

  if (script.IsEmpty()) {
    // check if this was an error of type
    // SyntaxError: Unexpected end of input
    // which we treat as a multiline mode trigger
    const auto message = _impl->to_string(try_catch.Exception());
    if (message == "SyntaxError: Unexpected end of input")
      *r_state = Input_state::ContinuedBlock;
    else
      _impl->print_exception(
          format_exception(get_v8_exception_data(try_catch, true)));
  } else {
    auto console = mysqlsh::current_console();

    v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(lcontext);
    if (is_terminating() || try_catch.HasTerminated()) {
      _impl->isolate->CancelTerminateExecution();
      console->print_diag("Script execution interrupted by user.");
    } else if (try_catch.HasCaught()) {
      Value exc(get_v8_exception_data(try_catch, true));
      if (exc)
        _impl->print_exception(format_exception(exc));
      else
        console->print_diag("Error executing script");
    } else {
      try {
        return {v8_value_to_shcore_value(result.ToLocalChecked()), false};
      } catch (std::exception &exc) {
        // we used to let the exception bubble up, but somehow, exceptions
        // thrown from v8_value_to_shcore_value() aren't being caught from
        // main.cc, leading to a crash due to unhandled exception.. so we catch
        // and print it here
        console->print_diag(std::string("INTERNAL ERROR: ").append(exc.what()));
      }
    }
  }
  return {Value(), true};
}

std::string JScript_context::format_exception(const shcore::Value &exc) {
  std::string error_message;

  if (exc.type == Map) {
    std::string type = exc.as_map()->get_string("type", "");
    std::string message = exc.as_map()->get_string("message", "");
    int64_t code = exc.as_map()->get_int("code", -1);
    std::string location = exc.as_map()->get_string("location", "");

    if (!message.empty()) {
      error_message += message;
      if (!type.empty() && code != -1)
        error_message += " (" + type + " " + std::to_string(code) + ")\n";
      else if (!type.empty())
        error_message += " (" + type + ")\n";
      else if (code != -1)
        error_message += " (" + std::to_string(code) + ") ";
      if (!location.empty()) error_message += " at " + location;
    }
  } else
    error_message = "Unexpected format of exception object.";

  if (error_message.length() &&
      error_message[error_message.length() - 1] != '\n')
    error_message += "\n";

  return error_message;
}

Value JScript_context::get_v8_exception_data(const v8::TryCatch &exc,
                                             bool interactive) {
  Value::Map_type_ref data;

  if (exc.Exception().IsEmpty() || exc.Exception()->IsUndefined())
    return Value();

  if (exc.Exception()->IsObject() &&
      JScript_map_wrapper::is_map(
          exc.Exception()->ToObject(context()).ToLocalChecked())) {
    data = _impl->types.v8_value_to_shcore_value(exc.Exception()).as_map();
  } else {
    const auto excstr = _impl->to_string(exc.Exception());
    data.reset(new Value::Map_type());
    if (!excstr.empty()) {
      // JS errors produced by V8 most likely will fall on this branch
      (*data)["message"] = Value(excstr);
    } else {
      (*data)["message"] = Value("Exception");
    }
  }

  bool include_location = !interactive;
  v8::Local<v8::Message> message = exc.Message();
  if (!message.IsEmpty()) {
    v8::Local<v8::Context> lcontext =
        v8::Local<v8::Context>::New(_impl->isolate, _impl->context);
    v8::Context::Scope context_scope(lcontext);

    // location
    const auto filename =
        _impl->to_string(message->GetScriptOrigin().ResourceName());
    std::string text = filename + ":";

    {
      const auto line_number = message->GetLineNumber(lcontext);
      text += (line_number.IsJust() ? std::to_string(line_number.FromJust())
                                    : "?") +
              std::string{":"};
    }

    {
      const auto start_column = message->GetStartColumn(lcontext);
      text += (start_column.IsJust() ? std::to_string(start_column.FromJust())
                                     : "?") +
              std::string{"\n"};
    }

    text.append("in ");

    {
      auto source_line = message->GetSourceLine(lcontext);

      if (!source_line.IsEmpty()) {
        text += _impl->to_string(source_line.ToLocalChecked());
      }

      text += "\n";
    }

    {
      const auto start_column = message->GetStartColumn(lcontext);
      const auto end_column = message->GetEndColumn(lcontext);

      if (start_column.IsJust() && end_column.IsJust()) {
        const auto start = start_column.FromJust();
        const auto end = end_column.FromJust();
        // underline
        text.append(3 + start, ' ');
        text.append(end - start, '^');
        text.append("\n");
      }
    }

    {
      auto stack_trace = exc.StackTrace(lcontext);

      if (!stack_trace.IsEmpty()) {
        const auto stack = _impl->to_string(stack_trace.ToLocalChecked());

        if (!stack.empty()) {
          auto new_lines = std::count(stack.begin(), stack.end(), '\n');
          if (new_lines > 1) {
            text.append(stack).append("\n");
            include_location = true;
          }
        }
      }
    }

    if (include_location) (*data)["location"] = Value(text);
  }

  return Value(data);
}

bool JScript_context::is_terminating() const { return _impl->is_terminating(); }

void JScript_context::terminate() { _impl->terminate(); }

v8::Local<v8::String> JScript_context::v8_string(const char *data) {
  return _impl->v8_string(data);
}

v8::Local<v8::String> JScript_context::v8_string(const std::string &data) {
  return v8_string(data.c_str());
}

std::string JScript_context::to_string(v8::Local<v8::Value> obj) {
  return _impl->to_string(obj);
}

std::string JScript_context::translate_exception(const v8::TryCatch &exc,
                                                 bool interactive) {
  return format_exception(get_v8_exception_data(exc, interactive));
}

void JScript_context::load_plugin(const std::string &file_name) {
  // load the file
  std::string source;

  if (load_text_file(file_name, source)) {
    const auto _isolate = isolate();

    v8::HandleScope handle_scope(_isolate);
    v8::TryCatch try_catch{_isolate};

    const auto new_context = _impl->copy_global_context();
    v8::Context::Scope context_scope(new_context);

    v8::ScriptOrigin script_origin{
        v8_string(shcore::path::basename(file_name))};
    v8::MaybeLocal<v8::Script> script =
        v8::Script::Compile(new_context, v8_string(source), &script_origin);

    if (script.IsEmpty()) {
      _impl->delete_context(new_context);
      log_error("Failed to compile JS plugin '%s'", file_name.c_str());
    } else {
      auto result = script.ToLocalChecked()->Run(new_context);

      // store the context even if an exception was thrown, we don't know when
      // exception was generated, script could have done something meaningful
      // before that happened
      _impl->store_context(new_context);

      if (result.IsEmpty()) {
        if (try_catch.HasCaught()) {
          log_error("Error while executing JS plugin '%s':\n%s",
                    file_name.c_str(),
                    translate_exception(try_catch, false).c_str());
        } else {
          log_error("Unknown error while executing JS plugin '%s'",
                    file_name.c_str());
        }
      }
    }
  } else {
    log_error("Failed to load JS plugin '%s'", file_name.c_str());
  }
}

v8::Local<v8::String> v8_string(v8::Isolate *isolate, const char *data) {
  return v8::String::NewFromUtf8(isolate, data, v8::NewStringType::kNormal)
      .ToLocalChecked();
}

v8::Local<v8::String> v8_string(v8::Isolate *isolate, const std::string &data) {
  return v8_string(isolate, data.c_str());
}

std::string to_string(v8::Isolate *isolate, v8::Local<v8::Value> obj) {
  const v8::String::Utf8Value utf8{isolate, obj};
  const auto ptr = *utf8;
  return nullptr == ptr ? "" : std::string(ptr, utf8.length());
}

}  // namespace shcore

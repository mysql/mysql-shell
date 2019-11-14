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

#include "my_config.h"

#if defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#if V8_MAJOR_VERSION > 6 || (V8_MAJOR_VERSION == 6 && V8_MINOR_VERSION > 7)
#error "v8::Platform uses deprecated code, remove these undefs when it's fixed"
#else
#undef V8_DEPRECATE_SOON
#define V8_DEPRECATE_SOON(message, declarator) declarator
#include <libplatform/libplatform.h>
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <list>
#include <stack>

#include "mysqlshdk/include/shellcore/console.h"
#include "scripting/module_registry.h"
#include "scripting/object_factory.h"
#include "scripting/object_registry.h"

#include "scripting/jscript_array_wrapper.h"
#include "scripting/jscript_function_wrapper.h"
#include "scripting/jscript_map_wrapper.h"
#include "scripting/jscript_object_wrapper.h"
#include "scripting/types_jscript.h"
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

using V8_args = v8::FunctionCallbackInfo<v8::Value>;

std::unique_ptr<v8::Platform> g_platform;

void print_exception(const std::string &text) {
  mysqlsh::current_console()->print_diag(text);
}

class Current_script {
 public:
  Current_script() : m_root(path::getcwd()) {
    // fake top-level script
    push("mysqlsh");
  }

  void push(const std::string &script) {
    m_scripts.push(path::is_absolute(script)
                       ? script
                       : path::normalize(path::join_path(m_root, script)));
  }

  void pop() { m_scripts.pop(); }

  std::string current_script() const { return m_scripts.top(); }

  std::string current_folder() const { return path::dirname(current_script()); }

 private:
  std::string m_root;
  std::stack<std::string> m_scripts;
};

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

class JScript_context::Impl {
 public:
  class Script_scope {
   public:
    Script_scope(Impl *owner, const std::string &s) : m_owner(owner) {
      m_owner->m_current_script.push(s);
    }

    Script_scope(const Script_scope &) = delete;
    Script_scope(Script_scope &&) = default;

    Script_scope &operator=(const Script_scope &) = delete;
    Script_scope &operator=(Script_scope &&) = default;

    ~Script_scope() {
      if (m_owner) {
        m_owner->m_current_script.pop();
      }
    }

   private:
    Impl *m_owner = nullptr;
  };

  explicit Impl(JScript_context *owner);
  ~Impl();

  static v8::Local<v8::String> v8_string(v8::Isolate *isolate,
                                         const char *data);
  static v8::Local<v8::String> v8_string(v8::Isolate *isolate,
                                         const std::string &data);
  static std::string to_string(v8::Isolate *isolate, v8::Local<v8::Value> obj);

  v8::Local<v8::String> v8_string(const char *data) const;
  v8::Local<v8::String> v8_string(const std::string &data) const;
  std::string to_string(v8::Local<v8::Value> obj) const;

  v8::Isolate *isolate() const;
  v8::Local<v8::Context> context() const;

  void set_global_item(const std::string &global_name,
                       const std::string &item_name,
                       const v8::Local<v8::Value> &value) const;
  void set_global(const std::string &name,
                  const v8::Local<v8::Value> &value) const;
  v8::Local<v8::Value> get_global(const std::string &name) const;

  void terminate();
  bool is_terminating() const;
  void clear_is_terminating();

  std::weak_ptr<JScript_function_storage> store(
      v8::Local<v8::Function> function);
  void erase(const std::shared_ptr<JScript_function_storage> &function);

  Script_scope enter_script(const std::string &s);

 private:
  // global functions exposed to JS
  static void f_repr(const V8_args &args);
  static void f_unrepr(const V8_args &args);
  static void f_type(const V8_args &args);
  static void f_print(const V8_args &args, bool new_line);
  static void f_source(const V8_args &args);
  // private global functions
  static void f_list_native_modules(const V8_args &args);
  static void f_load_native_module(const V8_args &args);
  static void f_load_module(const V8_args &args);
  static void f_current_module_folder(const V8_args &args);

  /*
   * load_core_module loads the content of the given module file
   * and inserts the definitions on the JS globals.
   */
  void load_core_module() const;
  void load_module(const std::string &path, v8::Local<v8::Value> module);

  v8::Local<v8::FunctionTemplate> wrap_callback(
      v8::FunctionCallback callback) const;

  v8::Local<v8::Context> copy_global_context() const;
  void delete_context(v8::Local<v8::Context> context) const;
  void store_context(v8::Local<v8::Context> context);

  Value convert(const v8::Local<v8::Value> &value) const;
  v8::Local<v8::Value> convert(const Value &value) const;
  v8::Local<v8::String> type_info(v8::Local<v8::Value> value) const;

  JScript_context *m_owner = nullptr;
  v8::Isolate *m_isolate = nullptr;
  v8::Persistent<v8::Context> m_context;
  std::unique_ptr<v8::ArrayBuffer::Allocator> m_allocator;
  bool m_terminating = false;
  std::vector<v8::Global<v8::Context>> m_stored_contexts;
  std::list<std::shared_ptr<JScript_function_storage>> m_stored_functions;
  Current_script m_current_script;
};

JScript_context::Impl::Impl(JScript_context *owner)
    : m_owner(owner),
      m_allocator(v8::ArrayBuffer::Allocator::NewDefaultAllocator()) {
  JScript_context_init();

  v8::Isolate::CreateParams params;
  params.array_buffer_allocator = m_allocator.get();

  m_isolate = v8::Isolate::New(params);
  m_isolate->SetData(0, this);

  v8::Isolate::Scope isolate_scope(m_isolate);
  v8::HandleScope handle_scope(m_isolate);

  v8::Local<v8::ObjectTemplate> globals = v8::ObjectTemplate::New(m_isolate);

  // register symbols to be exported to JS in global namespace

  // repr(object) -> string
  globals->Set(v8_string("repr"), wrap_callback(&Impl::f_repr));

  // unrepr(string) -> object
  globals->Set(v8_string("unrepr"), wrap_callback(&Impl::f_unrepr));

  // type(object) -> string
  globals->Set(v8_string("type"), wrap_callback(&Impl::f_type));

  // print('hello')
  globals->Set(v8_string("print"), wrap_callback([](const V8_args &args) {
                 f_print(args, false);
               }));

  // println('hello')
  globals->Set(v8_string("println"),
               wrap_callback([](const V8_args &args) { f_print(args, true); }));

  // source('module')
  globals->Set(v8_string("source"), wrap_callback(&Impl::f_source));

  // private functions
  globals->Set(v8_string("__list_native_modules"),
               wrap_callback(&Impl::f_list_native_modules));

  globals->Set(v8_string("__load_native_module"),
               wrap_callback(&Impl::f_load_native_module));

  globals->Set(v8_string("__load_module"), wrap_callback(&Impl::f_load_module));

  globals->Set(v8_string("__current_module_folder"),
               wrap_callback(&Impl::f_current_module_folder));

  // create the global context
  v8::Local<v8::Context> lcontext =
      v8::Context::New(m_isolate, nullptr, globals);
  m_context.Reset(m_isolate, lcontext);

  // Loads the core module
  load_core_module();
}

JScript_context::Impl::~Impl() {
  {
    v8::HandleScope handle_scope(m_isolate);

    for (const auto &c : m_stored_contexts) {
      delete_context(c.Get(m_isolate));
    }

    m_stored_contexts.clear();
  }

  m_stored_functions.clear();

  // release the context
  m_context.Reset();
  // notify the isolate
  m_isolate->ContextDisposedNotification();
  // force GC
  m_isolate->LowMemoryNotification();
  // dispose isolate
  m_isolate->Dispose();
}

void JScript_context::Impl::load_core_module() const {
  v8::Isolate::Scope isolate_scope(m_isolate);
  v8::HandleScope handle_scope(m_isolate);
  v8::TryCatch try_catch{m_isolate};
  const auto lcontext = context();
  v8::Context::Scope context_scope(lcontext);

  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

  v8::ScriptOrigin script_origin{v8_string("core.js")};
  auto script = v8::Script::Compile(
      lcontext, v8_string("(function (){" + shcore::js_core_module + "})();"),
      &script_origin);

  v8::MaybeLocal<v8::Value> result;
  if (!script.IsEmpty()) {
    result = script.ToLocalChecked()->Run(lcontext);
  }

  if (result.IsEmpty()) {
    throw shcore::Exception::runtime_error("Unable to load the core module!!");
  }
}

void JScript_context::Impl::load_module(const std::string &path,
                                        v8::Local<v8::Value> module) {
  const auto script_scope = enter_script(path);
  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

  // load the file
  std::string source;

  if (!load_text_file(path, source)) {
    throw std::runtime_error(
        shcore::str_format("Failed to open '%s'", path.c_str()));
  }

  // compile the source code
  v8::HandleScope handle_scope(m_isolate);
  v8::TryCatch try_catch{m_isolate};

  const auto new_context = copy_global_context();
  v8::Context::Scope context_scope(new_context);

  v8::ScriptOrigin script_origin{v8_string(path)};
  // immediately invoked function expression inside of another IIFE, this saves
  // some C++ code to extract the data from the module object
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(
      new_context,
      v8_string("(function(m){(function(exports,module,__filename,__dirname){" +
                source + "})(m.exports,m,m.__filename,m.__dirname)});"),
      &script_origin);

  if (script.IsEmpty()) {
    delete_context(new_context);

    throw std::runtime_error(
        shcore::str_format("Failed to compile '%s'.", path.c_str()));
  }

  // run the code, creating the function
  auto maybe_result = script.ToLocalChecked()->Run(new_context);

  const auto handle_empty_result = [this, &try_catch, &path]() {
    if (try_catch.HasCaught()) {
      // populate JS exception
      try_catch.ReThrow();
      // terminate execution
      throw std::runtime_error(shcore::str_format(
          "Failed to execute: %s",
          m_owner->translate_exception(try_catch, false).c_str()));
    } else {
      throw std::runtime_error(
          shcore::str_format("Failed to execute '%s'.", path.c_str()));
    }
  };

  if (maybe_result.IsEmpty()) {
    // nothing was executed yet, it's save to delete the context
    delete_context(new_context);
    handle_empty_result();
  }

  auto result = maybe_result.ToLocalChecked();

  if (!result->IsFunction()) {
    // this shouldn't happen, as it's our code which creates this function
    throw std::runtime_error(
        shcore::str_format("Failed to execute '%s'.", path.c_str()));
  }

  // execute the function (and user code), passing the module object
  auto function = v8::Local<v8::Function>::Cast(result);
  v8::Local<v8::Value> args[] = {module};

  maybe_result = function->Call(new_context, new_context->Global(),
                                shcore::array_size(args), args);

  // store the context even if an exception was thrown, we don't know when
  // exception was generated, script could have done something meaningful
  // before that happened
  store_context(new_context);

  if (maybe_result.IsEmpty()) {
    handle_empty_result();
  }
}

void JScript_context::Impl::f_repr(const V8_args &args) {
  const auto isolate = args.GetIsolate();
  const auto self = static_cast<Impl *>(isolate->GetData(0));

  v8::HandleScope handle_scope(isolate);

  if (args.Length() != 1) {
    isolate->ThrowException(v8_string(isolate, "repr() takes 1 argument"));
  } else {
    try {
      args.GetReturnValue().Set(
          v8_string(isolate, self->convert(args[0]).repr()));
    } catch (const std::exception &e) {
      isolate->ThrowException(v8_string(isolate, e.what()));
    }
  }
}

void JScript_context::Impl::f_unrepr(const V8_args &args) {
  const auto isolate = args.GetIsolate();
  const auto self = static_cast<Impl *>(isolate->GetData(0));

  v8::HandleScope handle_scope(isolate);

  if (args.Length() != 1) {
    isolate->ThrowException(v8_string(isolate, "unrepr() takes 1 argument"));
  } else {
    try {
      args.GetReturnValue().Set(
          self->convert(Value::parse(to_string(isolate, args[0]))));
    } catch (const std::exception &e) {
      isolate->ThrowException(v8_string(isolate, e.what()));
    }
  }
}

void JScript_context::Impl::f_type(const V8_args &args) {
  const auto isolate = args.GetIsolate();
  const auto self = static_cast<Impl *>(isolate->GetData(0));

  v8::HandleScope handle_scope(isolate);

  if (args.Length() != 1) {
    isolate->ThrowException(v8_string(isolate, "type() takes 1 argument"));
  } else {
    try {
      args.GetReturnValue().Set(self->type_info(args[0]));
    } catch (const std::exception &e) {
      isolate->ThrowException(v8_string(isolate, e.what()));
    }
  }
}

void JScript_context::Impl::f_print(const V8_args &args, bool new_line) {
  const auto isolate = args.GetIsolate();
  const auto self = static_cast<Impl *>(isolate->GetData(0));

  v8::HandleScope outer_handle_scope(isolate);

  std::string text;
  // FIXME this doesn't belong here?
  std::string format = mysqlsh::current_shell_options()->get().wrap_json;

  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(isolate);
    if (i > 0) text.push_back(' ');

    try {
      if (format.find("json") == 0)
        text += self->convert(args[i]).json(format == "json");
      else
        text += self->convert(args[i]).descr(true);
    } catch (const std::exception &e) {
      isolate->ThrowException(v8_string(isolate, e.what()));
      break;
    }
  }
  if (new_line) text.append("\n");

  mysqlsh::current_console()->print(text);
}

void JScript_context::Impl::f_source(const V8_args &args) {
  const auto isolate = args.GetIsolate();
  const auto self = static_cast<Impl *>(isolate->GetData(0));

  v8::HandleScope outer_handle_scope(isolate);

  if (args.Length() != 1) {
    isolate->ThrowException(v8_string(isolate, "Invalid number of parameters"));
    return;
  }

  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

  v8::HandleScope handle_scope(isolate);
  const auto str = to_string(isolate, args[0]);

  // Loads the source content
  std::string source;
  if (load_text_file(str, source)) {
    self->m_owner->execute(source, str, {});
  } else {
    isolate->ThrowException(v8_string(isolate, "Error loading script"));
  }
}

void JScript_context::Impl::f_list_native_modules(const V8_args &args) {
  const auto isolate = args.GetIsolate();

  v8::HandleScope handle_scope(isolate);

  if (args.Length() != 0) {
    isolate->ThrowException(v8_string(isolate, "Invalid number of parameters"));
  } else {
    const auto core_modules = Object_factory::package_contents("__modules__");
    const auto size = static_cast<int>(core_modules.size());
    const auto array = v8::Array::New(isolate, size);

    for (int i = 0; i < size; ++i) {
      array
          ->Set(isolate->GetCurrentContext(), i,
                v8_string(isolate, core_modules[i]))
          .FromJust();
    }

    args.GetReturnValue().Set(array);
  }
}

void JScript_context::Impl::f_load_native_module(const V8_args &args) {
  const auto isolate = args.GetIsolate();
  const auto self = static_cast<Impl *>(isolate->GetData(0));

  v8::HandleScope handle_scope(isolate);

  if (args.Length() != 1) {
    isolate->ThrowException(v8_string(isolate, "Invalid number of parameters"));
  } else {
    try {
      const auto s = to_string(isolate, args[0]);
      shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

      const auto core_modules = Object_factory::package_contents("__modules__");
      if (std::find(core_modules.begin(), core_modules.end(), s) !=
          core_modules.end()) {
        const auto module = Object_factory::call_constructor(
            "__modules__", s, shcore::Argument_list());
        args.GetReturnValue().Set(self->convert(shcore::Value(module)));
      }
    } catch (const std::exception &e) {
      isolate->ThrowException(v8_string(isolate, e.what()));
    }
  }
}

void JScript_context::Impl::f_load_module(const V8_args &args) {
  const auto isolate = args.GetIsolate();
  const auto self = static_cast<Impl *>(isolate->GetData(0));

  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch{isolate};

  if (args.Length() != 2) {
    isolate->ThrowException(v8_string(isolate, "Invalid number of parameters"));
  } else {
    try {
      self->load_module(to_string(isolate, args[0]), args[1]);
    } catch (const std::runtime_error &ex) {
      // throw new JS exception only if there isn't one already
      if (!try_catch.HasCaught()) {
        // needed to create the v8 exception
        v8::Isolate::Scope isolate_scope(isolate);
        isolate->ThrowException(
            v8::Exception::Error(v8_string(isolate, ex.what())));
      }
    }
  }

  // ensure that JS exception is populated
  if (try_catch.HasCaught()) {
    try_catch.ReThrow();
  }
}

void JScript_context::Impl::f_current_module_folder(const V8_args &args) {
  const auto isolate = args.GetIsolate();
  const auto self = static_cast<Impl *>(isolate->GetData(0));

  v8::HandleScope handle_scope(isolate);

  if (args.Length() != 0) {
    isolate->ThrowException(v8_string(isolate, "Invalid number of parameters"));
  } else {
    args.GetReturnValue().Set(
        v8_string(isolate, self->m_current_script.current_folder()));
  }
}

void JScript_context::Impl::set_global_item(
    const std::string &global_name, const std::string &item_name,
    const v8::Local<v8::Value> &value) const {
  v8::HandleScope handle_scope(m_isolate);
  v8::Local<v8::Value> global = get_global(global_name);
  v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(global);
  const auto lcontext = context();

  object->Set(lcontext, v8_string(item_name), value).FromJust();
}

void JScript_context::Impl::set_global(
    const std::string &name, const v8::Local<v8::Value> &value) const {
  v8::HandleScope handle_scope(m_isolate);
  const auto ctx = context();

  if (value.IsEmpty() || !*value)
    ctx->Global()->Set(ctx, v8_string(name), v8::Null(m_isolate)).FromJust();
  else
    ctx->Global()->Set(ctx, v8_string(name), value).FromJust();
}

v8::Local<v8::Value> JScript_context::Impl::get_global(
    const std::string &name) const {
  const auto ctx = context();
  return ctx->Global()->Get(ctx, v8_string(name)).ToLocalChecked();
}

v8::Local<v8::String> JScript_context::Impl::v8_string(v8::Isolate *isolate,
                                                       const char *data) {
  return shcore::v8_string(isolate, data);
}

v8::Local<v8::String> JScript_context::Impl::v8_string(
    v8::Isolate *isolate, const std::string &data) {
  return v8_string(isolate, data.c_str());
}

v8::Local<v8::String> JScript_context::Impl::v8_string(const char *data) const {
  return v8_string(m_isolate, data);
}

v8::Local<v8::String> JScript_context::Impl::v8_string(
    const std::string &data) const {
  return v8_string(data.c_str());
}

std::string JScript_context::Impl::to_string(v8::Isolate *isolate,
                                             v8::Local<v8::Value> obj) {
  return shcore::to_string(isolate, obj);
}

std::string JScript_context::Impl::to_string(v8::Local<v8::Value> obj) const {
  return to_string(m_isolate, obj);
}

void JScript_context::Impl::terminate() {
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
  m_isolate->TerminateExecution();
}

bool JScript_context::Impl::is_terminating() const { return m_terminating; }

void JScript_context::Impl::clear_is_terminating() { m_terminating = false; }

v8::Local<v8::FunctionTemplate> JScript_context::Impl::wrap_callback(
    v8::FunctionCallback callback) const {
  const auto data =
      v8::External::New(m_isolate, reinterpret_cast<void *>(callback));

  return v8::FunctionTemplate::New(
      m_isolate,
      [](const V8_args &args) {
        const auto isolate = args.GetIsolate();
        const auto self = static_cast<Impl *>(isolate->GetData(0));

        if (self->is_terminating()) return;

        const auto func = reinterpret_cast<v8::FunctionCallback>(
            v8::External::Cast(*args.Data())->Value());
        func(args);
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
v8::Local<v8::Context> JScript_context::Impl::copy_global_context() const {
  const auto new_context = v8::Context::New(m_isolate);

  // copy globals into the new context
  const auto origin = m_context.Get(m_isolate);

  const auto globals = origin->Global();
  const auto names = globals->GetPropertyNames(origin).ToLocalChecked();

  const auto new_globals = new_context->Global();

  for (uint32_t c = names->Length(), i = 0; i < c; ++i) {
    const auto name = names->Get(origin, i).ToLocalChecked();
    new_globals
        ->Set(new_context, name, globals->Get(origin, name).ToLocalChecked())
        .FromJust();
  }

  return new_context;
}

void JScript_context::Impl::delete_context(
    v8::Local<v8::Context> context) const {
  const auto globals = context->Global();
  const auto names = globals->GetPropertyNames(context).ToLocalChecked();

  for (uint32_t c = names->Length(), i = 0; i < c; ++i) {
    const auto name = names->Get(context, i).ToLocalChecked();

    if (globals->Delete(context, name).IsNothing()) {
      log_error("Failed to delete global JS: %s", to_string(name).c_str());
    }
  }
}

void JScript_context::Impl::store_context(v8::Local<v8::Context> context) {
  m_stored_contexts.emplace_back(m_isolate, context);
}

std::weak_ptr<JScript_function_storage> JScript_context::Impl::store(
    v8::Local<v8::Function> function) {
  m_stored_functions.emplace_back(
      std::make_shared<JScript_function_storage>(m_isolate, function));
  return m_stored_functions.back();
}

void JScript_context::Impl::erase(
    const std::shared_ptr<JScript_function_storage> &function) {
  m_stored_functions.remove(function);
}

v8::Isolate *JScript_context::Impl::isolate() const { return m_isolate; }

v8::Local<v8::Context> JScript_context::Impl::context() const {
  return v8::Local<v8::Context>::New(isolate(), m_context);
}

Value JScript_context::Impl::convert(const v8::Local<v8::Value> &value) const {
  return m_owner->convert(value);
}

v8::Local<v8::Value> JScript_context::Impl::convert(const Value &value) const {
  return m_owner->convert(value);
}

v8::Local<v8::String> JScript_context::Impl::type_info(
    v8::Local<v8::Value> value) const {
  return m_owner->type_info(value);
}

JScript_context::Impl::Script_scope JScript_context::Impl::enter_script(
    const std::string &s) {
  return Script_scope(this, s);
}

JScript_context::JScript_context(Object_registry *registry)
    : m_impl(std::make_unique<Impl>(this)),
      m_types(std::make_unique<JScript_type_bridger>(this)) {
  // initialize type conversion
  m_types->init();

  set_global("globals", Value(registry->_registry));
}

JScript_context::~JScript_context() { m_types->dispose(); }

void JScript_context::set_global_item(const std::string &global_name,
                                      const std::string &item_name,
                                      const Value &value) {
  // makes isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(isolate());
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(isolate());
  // catch everything that happens in this scope
  v8::TryCatch try_catch{isolate()};
  // set context to be the default context for everything in this scope
  v8::Context::Scope context_scope(context());

  m_impl->set_global_item(global_name, item_name, convert(value));
}

void JScript_context::set_global(const std::string &name, const Value &value) {
  // makes isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(isolate());
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(isolate());
  // catch everything that happens in this scope
  v8::TryCatch try_catch{isolate()};
  // set context to be the default context for everything in this scope
  v8::Context::Scope context_scope(context());

  m_impl->set_global(name, convert(value));
}

Value JScript_context::get_global(const std::string &name) {
  // makes isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(isolate());
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(isolate());
  // catch everything that happens in this scope
  v8::TryCatch try_catch{isolate()};
  // set context to be the default context for everything in this scope
  v8::Context::Scope context_scope(context());

  return convert(m_impl->get_global(name));
}

std::tuple<JSObject, std::string> JScript_context::get_global_js(
    const std::string &name) {
  // makes isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(isolate());
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(isolate());
  // catch everything that happens in this scope
  v8::TryCatch try_catch{isolate()};
  // set context to be the default context for everything in this scope
  const auto lcontext = context();
  v8::Context::Scope context_scope(lcontext);

  v8::Local<v8::Value> global(m_impl->get_global(name));
  if (global->IsObject()) {
    auto obj_type = m_impl->to_string(type_info(global));
    if (shcore::str_beginswith(obj_type, "m.")) obj_type = obj_type.substr(2);
    return std::tuple<JSObject, std::string>(
        JSObject(isolate(), global->ToObject(lcontext).ToLocalChecked()),
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
  // makes isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(isolate());
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(isolate());
  // catch everything that happens in this scope
  v8::TryCatch try_catch{isolate()};
  // set context to be the default context for everything in this scope
  const auto lcontext = context();
  v8::Context::Scope context_scope(lcontext);

  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

  v8::Local<v8::Object> lobj(v8::Local<v8::Object>::New(isolate(), *obj));

  auto maybe_value = lobj->Get(lcontext, v8_string(name));
  if (!maybe_value.IsEmpty()) {
    const auto value = maybe_value.ToLocalChecked();

    auto type = m_impl->to_string(type_info(value));
    if (shcore::str_beginswith(type, "m.")) {
      type = type.substr(2);
    }

    if (value->IsFunction()) {
      return std::make_tuple(true, JSObject(), type);
    } else if (value->IsObject()) {
      const auto object = value->ToObject(lcontext).ToLocalChecked();
      return std::make_tuple(object->IsCallable(), JSObject(isolate(), object),
                             type);
    }
  }

  return std::make_tuple(false, JSObject(), "");
}

std::vector<std::pair<bool, std::string>> JScript_context::get_members_of(
    const JSObject *obj) {
  // makes isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(isolate());
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(isolate());
  // catch everything that happens in this scope
  v8::TryCatch try_catch{isolate()};
  // set context to be the default context for everything in this scope
  const auto lcontext = context();
  v8::Context::Scope context_scope(lcontext);

  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

  v8::Local<v8::Object> lobj(v8::Local<v8::Object>::New(isolate(), *obj));

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
             m_impl->to_string(props->Get(lcontext, i).ToLocalChecked())});
      }
    }
  }
  return keys;
}

v8::Isolate *JScript_context::isolate() const { return m_impl->isolate(); }

v8::Local<v8::Context> JScript_context::context() const {
  return m_impl->context();
}

Value JScript_context::convert(const v8::Local<v8::Value> &value) const {
  return m_types->v8_value_to_shcore_value(value);
}

v8::Local<v8::Value> JScript_context::convert(const Value &value) const {
  return m_types->shcore_value_to_v8_value(value);
}

Argument_list JScript_context::convert_args(const V8_args &args) {
  Argument_list r;

  for (int c = args.Length(), i = 0; i < c; i++) {
    r.push_back(convert(args[i]));
  }

  return r;
}

v8::Local<v8::String> JScript_context::type_info(
    v8::Local<v8::Value> value) const {
  return m_types->type_info(value);
}

std::pair<Value, bool> JScript_context::execute(
    const std::string &code_str, const std::string &source,
    const std::vector<std::string> &argv) {
  // makes isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(isolate());
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(isolate());
  // catch everything that happens in this scope
  v8::TryCatch try_catch{isolate()};
  // set context to be the default context for everything in this scope
  v8::Local<v8::Context> lcontext = context();
  v8::Context::Scope context_scope(lcontext);
  v8::ScriptOrigin origin(v8_string(source));
  v8::Local<v8::String> code = v8_string(code_str);
  v8::MaybeLocal<v8::Script> script =
      v8::Script::Compile(lcontext, code, &origin);

  const auto script_scope = m_impl->enter_script(source);

  // Since ret_val can't be used to check whether all was ok or not
  // Will use a boolean flag

  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

  shcore::Value args_value = get_global("sys").as_object()->get_member("argv");
  auto args = args_value.as_array();
  args->clear();
  for (auto &arg : argv) {
    args->push_back(Value(arg));
  }

  m_impl->clear_is_terminating();

  if (!script.IsEmpty()) {
    v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(lcontext);
    if (is_terminating() || try_catch.HasTerminated()) {
      isolate()->CancelTerminateExecution();
      mysqlsh::current_console()->print_diag(
          "Script execution interrupted by user.");

      return {Value(), true};
    } else if (!try_catch.HasCaught()) {
      return {convert(result.ToLocalChecked()), false};
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
  // makes isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(isolate());
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(isolate());
  // catch everything that happens in this scope
  v8::TryCatch try_catch{isolate()};
  // set context to be the default context for everything in this scope
  v8::Local<v8::Context> lcontext = context();
  v8::Context::Scope context_scope(lcontext);
  v8::ScriptOrigin origin(v8_string("(shell)"));
  v8::Local<v8::String> code = v8_string(code_str);
  v8::MaybeLocal<v8::Script> script =
      v8::Script::Compile(lcontext, code, &origin);

  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

  m_impl->clear_is_terminating();

  *r_state = Input_state::Ok;

  if (script.IsEmpty()) {
    // check if this was an error of type
    // - SyntaxError: Unexpected end of input
    // - SyntaxError: Unterminated template literal
    // which we treat as a multiline mode trigger or
    // - SyntaxError: Invalid or unexpected token
    // which may be a sign of unfinished C style comment
    const char *unexpected_end_exc = "SyntaxError: Unexpected end of input";
    const char *unterminated_template_exc =
        "SyntaxError: Unterminated template literal";
    const char *unexpected_tok_exc = "SyntaxError: Invalid or unexpected token";
    auto message = m_impl->to_string(try_catch.Exception());
    if (message == unexpected_end_exc || message == unterminated_template_exc) {
      *r_state = Input_state::ContinuedBlock;
    } else if (message == unexpected_tok_exc) {
      auto comment_pos = code_str.rfind("/*");
      while (comment_pos != std::string::npos &&
             code_str.find("*/", comment_pos + 2) == std::string::npos) {
        try_catch.Reset();
        if (!v8::Script::Compile(
                 lcontext, v8_string(code_str.substr(0, comment_pos)), &origin)
                 .IsEmpty()) {
          *r_state = Input_state::ContinuedSingle;
        } else {
          message = m_impl->to_string(try_catch.Exception());
          if (message == unexpected_end_exc) {
            *r_state = Input_state::ContinuedBlock;
          } else if (message == unexpected_tok_exc && comment_pos > 0) {
            comment_pos = code_str.rfind("/*", comment_pos - 1);
            continue;
          }
        }
        break;
      }
    }
    if (*r_state == Input_state::Ok)
      print_exception(format_exception(get_v8_exception_data(try_catch, true)));
  } else {
    auto console = mysqlsh::current_console();

    v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(lcontext);
    if (is_terminating() || try_catch.HasTerminated()) {
      isolate()->CancelTerminateExecution();
      console->print_diag("Script execution interrupted by user.");
    } else if (try_catch.HasCaught()) {
      Value exc(get_v8_exception_data(try_catch, true));
      if (exc)
        print_exception(format_exception(exc));
      else
        console->print_diag("Error executing script");
    } else {
      try {
        return {convert(result.ToLocalChecked()), false};
      } catch (const std::exception &exc) {
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

  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

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
  } else {
    error_message = "Unexpected format of exception object.";
  }

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

  shcore::Scoped_naming_style style(NamingStyle::LowerCamelCase);

  if (exc.Exception()->IsObject() &&
      JScript_map_wrapper::is_map(
          exc.Exception()->ToObject(context()).ToLocalChecked())) {
    data = convert(exc.Exception()).as_map();
  } else {
    const auto excstr = m_impl->to_string(exc.Exception());
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
    const auto lcontext = context();
    v8::Context::Scope context_scope(lcontext);

    // location
    const auto filename =
        m_impl->to_string(message->GetScriptOrigin().ResourceName());
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
        text += m_impl->to_string(source_line.ToLocalChecked());
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
        const auto stack = m_impl->to_string(stack_trace.ToLocalChecked());

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

bool JScript_context::is_terminating() const {
  return m_impl->is_terminating();
}

void JScript_context::terminate() { m_impl->terminate(); }

v8::Local<v8::String> JScript_context::v8_string(const char *data) {
  return m_impl->v8_string(data);
}

v8::Local<v8::String> JScript_context::v8_string(const std::string &data) {
  return v8_string(data.c_str());
}

std::string JScript_context::to_string(v8::Local<v8::Value> obj) {
  return m_impl->to_string(obj);
}

std::string JScript_context::translate_exception(const v8::TryCatch &exc,
                                                 bool interactive) {
  return format_exception(get_v8_exception_data(exc, interactive));
}

bool JScript_context::load_plugin(const Plugin_definition &plugin) {
  bool ret_val = true;
  const auto &file_name = plugin.file;

  const auto iso = isolate();
  v8::HandleScope handle_scope(iso);
  v8::TryCatch try_catch{iso};
  const auto lcontext = context();
  v8::Context::Scope context_scope(lcontext);

  v8::ScriptOrigin script_origin{v8_string("(load_plugin)")};
  v8::MaybeLocal<v8::Script> script =
      v8::Script::Compile(lcontext,
                          v8_string("require.__mh.__load_module(" +
                                    shcore::quote_string(file_name, '"') + ")"),
                          &script_origin);

  if (script.IsEmpty()) {
    ret_val = false;
    log_error("Error loading JavaScript file '%s':\n\t%s", file_name.c_str(),
              "Failed to compile.");
  } else {
    const auto result = script.ToLocalChecked()->Run(lcontext);

    if (result.IsEmpty()) {
      ret_val = false;
      if (try_catch.HasCaught()) {
        log_error("Error loading JavaScript file '%s':\n\tExecution failed: %s",
                  file_name.c_str(),
                  translate_exception(try_catch, false).c_str());
      } else {
        log_error("Error loading JavaScript file '%s':\n\tUnknown error",
                  file_name.c_str());
      }
    }
  }

  return ret_val;
}

std::weak_ptr<JScript_function_storage> JScript_context::store(
    v8::Local<v8::Function> function) {
  return m_impl->store(function);
}

void JScript_context::erase(
    const std::shared_ptr<JScript_function_storage> &function) {
  m_impl->erase(function);
}

v8::Local<v8::String> v8_string(v8::Isolate *isolate, const char *data) {
  return v8::String::NewFromUtf8(isolate, data, v8::NewStringType::kNormal)
      .ToLocalChecked();
}

v8::Local<v8::String> v8_string(v8::Isolate *isolate, const std::string &data) {
  return v8_string(isolate, data.c_str());
}

std::string to_string(v8::Isolate *isolate, v8::Local<v8::Value> obj) {
  const v8::String::Utf8Value utf8{
      isolate, obj->IsSymbol() ? obj.As<v8::Symbol>()->Name() : obj};
  const auto ptr = *utf8;
  return nullptr == ptr ? "" : std::string(ptr, utf8.length());
}

}  // namespace shcore

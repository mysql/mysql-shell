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

#include "shellcore/jscript_context.h"
#include "shellcore/lang_base.h"
#include "shellcore/shell_core.h"

#include <include/v8.h>

#include <iostream>

using namespace shcore;


struct JScript_context::JScript_context_impl
{
  v8::Isolate *isolate;
  v8::Eternal<v8::Context> context;

  Interpreter_delegate *delegate;

  JScript_context_impl(Interpreter_delegate *deleg)
  : isolate(v8::Isolate::New()), delegate(deleg)
  {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    v8::Handle<v8::ObjectTemplate> globals = v8::ObjectTemplate::New(isolate);
    v8::Local<v8::External> client_data(v8::External::New(isolate, this));

    // register symbols to be exported to JS in global namespace
    globals->Set(v8::String::NewFromUtf8(isolate, "print"),
                 v8::FunctionTemplate::New(isolate, &JScript_context_impl::f_print, client_data));


    context.Set(isolate, v8::Context::New(isolate, NULL, globals));
  }

  static void f_print(const v8::FunctionCallbackInfo<v8::Value>& args)
  {
    v8::HandleScope outer_handle_scope(args.GetIsolate());
    JScript_context_impl *self = static_cast<JScript_context_impl*>(v8::External::Cast(*args.Data())->Value());

    for (int i = 0; i < args.Length(); i++)
    {
      v8::HandleScope handle_scope(args.GetIsolate());
      v8::String::Utf8Value str(args[i]);
      if (i > 0)
        self->delegate->print(self->delegate->user_data, " ");
      self->delegate->print(self->delegate->user_data, *str);
    }
  }
};


void JScript_context::init()
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


JScript_context::JScript_context(Interpreter_delegate *deleg)
: _impl(new JScript_context_impl(deleg))
{
}



JScript_context::~JScript_context()
{
  delete _impl;
}


static void dump_exception(v8::TryCatch *exc)
{
  v8::String::Utf8Value exception(exc->Exception());

  if (const char *text = *exception)
    std::cerr << "V8 Exception: " << text << "\n";
}


Value JScript_context::execute(const std::string &code_str, boost::system::error_code &ret_error)
{
  // makes _isolate the default isolate for this context
  v8::Isolate::Scope isolate_scope(_impl->isolate);
  // creates a pool for all the handles that are created in this scope
  // (except for persistent ones), which will be freed when the scope exits
  v8::HandleScope handle_scope(_impl->isolate);
  // catch everything that happens in this scope
  v8::TryCatch try_catch;
  // set _context to be the default context for everything in this scope
  v8::Context::Scope context_scope(_impl->context.Get(_impl->isolate));
  v8::ScriptOrigin origin(v8::String::NewFromUtf8(_impl->isolate, "(shell)"));
  v8::Handle<v8::String> code = v8::String::NewFromUtf8(_impl->isolate, code_str.c_str());
  v8::Handle<v8::Script> script = v8::Script::Compile(code, &origin);

  if (script.IsEmpty())
  {
    dump_exception(&try_catch);
    return Value();
  }
  else
  {
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty())
    {
      dump_exception(&try_catch);
      return Value();
    }
    else
    {
      if (!result->IsUndefined())
      {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        v8::String::Utf8Value str(result);
        std::cout << ">>"<<*str << "\n";
      }
      return Value();
    }
  }
  return Value();
}


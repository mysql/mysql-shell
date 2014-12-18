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

//#include <v8/include/libplatform/libplatform.h>

using namespace shcore;
using namespace v8;

void JScript_context::init()
{
  static bool inited = false;
  if (!inited)
  {
//    V8::InitializeICU();
//    Platform* platform = v8::platform::CreateDefaultPlatform();
//    V8::InitializePlatform(platform);
    V8::Initialize();
  }
}

static struct _ {
  _() { JScript_context::init(); }
} autoinit;


JScript_context::JScript_context()
: _isolate(Isolate::New()), _isolate_scope(_isolate)
{
  v8::HandleScope handle_scope(_isolate);
  Handle<ObjectTemplate> globals = ObjectTemplate::New(_isolate);

  _context = Context::New(_isolate, NULL, globals);
}



bool JScript_context::execute_code(const std::string &code_str)
{
  v8::HandleScope handle_scope(_isolate);
  v8::TryCatch try_catch;
  v8::ScriptOrigin origin(v8::String::NewFromUtf8(_isolate, code_str.c_str()));
  v8::Handle<v8::String> code = v8::String::NewFromUtf8(_isolate, code_str.c_str());
  v8::Handle<v8::Script> script = v8::Script::Compile(code, &origin);
  if (script.IsEmpty())
  {
//    ReportException(_isolate, &try_catch);
    return false;
  }
  else
  {
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty())
    {
      // Print errors that happened during execution.
      // ReportException(_isolate, &try_catch);
      return false;
    }
    else
    {
      if (!result->IsUndefined())
      {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        v8::String::Utf8Value str(result);
      }
      return true;
    }
  }

  return true;
}


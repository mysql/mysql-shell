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

#ifndef _JSCRIPT_CONTEXT_H_
#define _JSCRIPT_CONTEXT_H_

#include <include/v8.h>
#include <string>

namespace shcore {

class JScript_context
{
public:
  static void init();

  JScript_context();

  bool execute_code(const std::string &code);

private:
  v8::Isolate *_isolate;
  v8::Isolate::Scope _isolate_scope;
  v8::Handle<v8::Context> _context;
};

};

#endif

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

#ifndef _SHELLCORE_JS_H_
#define _SHELLCORE_JS_H_

#include "shellcore/shell_core.h"

#include <v8/v8.h>

namespace shcore {

class JS_context : public Language_context
{
public:
  JS_context(Registry *reg);



private:
  v8::Local<v8::Context> _context;
};



class JS_function : public Function_base
{
public:
private:
  v8::Local<v8::ObjectTemplate> _obj;
};

};

#endif


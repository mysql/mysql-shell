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

#ifndef _TYPES_JSCRIPT_H_
#define _TYPES_JSCRIPT_H_

#include "shellcore/jscript_context.h"

#include "shellcore/types.h"


class JScript_function : public Function_base
{
public:
  virtual std::string name();

  virtual std::vector<std::pair<std::string, Value_type> > signature();

  virtual std::pair<std::string, Value_type> return_type();

  virtual bool operator == (const Function_base &other) const;

  virtual bool operator != (const Function_base &other) const;

  virtual bool invoke(const std::vector<Value> &args, Value &return_value);

private:
  boost::shared_ptr<JScript_context> _js;
  v8::Local<v8::ObjectTemplate> _obj;
};


#endif

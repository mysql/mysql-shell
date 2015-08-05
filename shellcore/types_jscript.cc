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


#include "shellcore/types_jscript.h"
#include "shellcore/object_factory.h"
#include "shellcore/common.h"

#include <v8.h>
#include <boost/weak_ptr.hpp>

using namespace shcore;


class JScript_object : public Object_bridge
{
public:
  JScript_object(boost::shared_ptr<JScript_context> UNUSED(context))
  {
  }

  virtual ~JScript_object()
  {
  }

  virtual std::string &append_descr(std::string &s_out, int UNUSED(indent)=-1, int UNUSED(quote_strings)=0) const
  {
    return s_out;
  }

  virtual std::string &append_repr(std::string &s_out) const
  {
    return s_out;
  }

  virtual std::vector<std::string> get_members() const
  {
    std::vector<std::string> mlist;
    return mlist;
  }

  virtual bool operator == (const Object_bridge &UNUSED(other)) const
  {
    return false;
  }

  virtual bool operator != (const Object_bridge &UNUSED(other)) const
  {
    return false;
  }

  virtual Value get_property(const std::string &UNUSED(prop)) const
  {
    return Value();
  }

  virtual void set_property(const std::string &UNUSED(prop), Value UNUSED(value))
  {
  }

  virtual Value call(const std::string &name, const Argument_list &args)
  {
    v8::Handle<v8::Value> member(_object->Get(v8::String::NewFromUtf8(_js->isolate(), name.c_str())));
    if (member->IsFunction())
    {
      v8::Handle<v8::Function> f;
      f.Cast(member);
      std::vector<v8::Handle<v8::Value> > argv;

      //XX convert the arg list

      v8::Handle<v8::Value> result = f->Call(_js->context()->Global(), (int)args.size(), &argv[0]);
      return _js->v8_value_to_shcore_value(result);
    }
    else
    {
      throw Exception::attrib_error("Called member "+name+" of JS object is not a function");
    }
  }
  
private:
  boost::shared_ptr<JScript_context> _js;
  v8::Handle<v8::Object> _object;
};

// -------------------------------------------------------------------------------------------------------

class JScript_object_factory : public Object_factory
{
public:
  JScript_object_factory(boost::shared_ptr<JScript_context> context, v8::Handle<v8::Object> constructor);

  virtual boost::shared_ptr<Object_bridge> construct(const Argument_list &UNUSED(args))
  {
    return construct_from_repr("");
  }

  virtual boost::shared_ptr<Object_bridge> construct_from_repr(const std::string &UNUSED(repr))
  {
    return boost::shared_ptr<Object_bridge>();
  }

private:
  boost::weak_ptr<JScript_context> _context;
};

// -------------------------------------------------------------------------------------------------------


JScript_function::JScript_function(boost::shared_ptr<JScript_context> context)
: _js(context)
{
throw std::logic_error("not implemented");
}

std::string JScript_function::name()
{
  // TODO:
  return "";
}

std::vector<std::pair<std::string, Value_type> > JScript_function::signature()
{
  // TODO:
  return std::vector<std::pair<std::string, Value_type> >();
}

std::pair<std::string, Value_type> JScript_function::return_type()
{
  // TODO:
  return std::pair<std::string, Value_type>();
}

bool JScript_function::operator == (const Function_base &UNUSED(other)) const
{
  // TODO:
  return false;
}

bool JScript_function::operator != (const Function_base &UNUSED(other)) const
{
  // TODO:
  return false;
}

Value JScript_function::invoke(const Argument_list &UNUSED(args))
{
  // TODO:
  return Value();
}

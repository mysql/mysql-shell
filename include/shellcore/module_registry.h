/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _MODULE_REGISTRY_H_
#define _MODULE_REGISTRY_H_

#include "shellcore/types_cpp.h"
#include "shellcore/object_factory.h"

namespace shcore
{
  class SHCORE_PUBLIC Module_base : public shcore::Cpp_object_bridge
  {
    // This class is to only allow exporting as modules the objects that are meant to
    // be modules
  };

#define DECLARE_MODULE(C,N) \
class SHCORE_PUBLIC C : public shcore::Module_base \
                                                  {\
        public:\
          C(); \
          void init(); \
          virtual std::string class_name() const { return #N; };\
          virtual bool operator == (const Object_bridge &other) const{ return false; }\
          static boost::shared_ptr<shcore::Object_bridge> create(const shcore::Argument_list &args)

#define DECLARE_FUNCTION(F) shcore::Value F(const shcore::Argument_list &args);

#define END_DECLARE_MODULE() }

#define REGISTER_MODULE(C,N) \
  shcore::Module_register<C>C ## _ ## N ## _register(#N); \
  C::C() { init(); } \
  boost::shared_ptr<shcore::Object_bridge> C::create(const shcore::Argument_list &args) \
                              { \
    return boost::shared_ptr<shcore::Object_bridge>(new C()); \
                              } \
  void C::init()

#define REGISTER_FUNCTION(C,F,N, ...) add_method(#N, boost::bind(&C::F, this, _1), __VA_ARGS__)
#define REGISTER_VARARGS_FUNCTION(C,F,N) add_varargs_method(#N, boost::bind(&C::F, this, _1))

#define END_REGISTER_MODULE() }

#define DEFINE_FUNCTION(C,F) shcore::Value C::F(const shcore::Argument_list &args)

#define INIT_MODULE(X) {X a;}

  template<class ObjectBridgeClass>
  struct Module_register
  {
    Module_register(const std::string &name)
    {
      shcore::Object_factory::register_factory("__modules__", name, &ObjectBridgeClass::create);
    }
  };
};
#endif //_MODULE_REGISTRY_H_

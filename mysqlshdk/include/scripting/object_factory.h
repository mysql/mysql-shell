/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _OBJECT_FACTORY_H_
#define _OBJECT_FACTORY_H_

#include "scripting/types_common.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h" // NamingStyle

namespace shcore {
class SHCORE_PUBLIC Object_factory {
public:
  typedef std::shared_ptr<Object_bridge>(*Factory_function)(const Argument_list &args, NamingStyle style);

  //! Registers a metaclass
  static void register_factory(const std::string &package, const std::string &class_name,
                               Factory_function function);

  static std::shared_ptr<Object_bridge> call_constructor(const std::string &package, const std::string &name,
                                                           const Argument_list &args, NamingStyle style);

  static std::vector<std::string> package_names();
  static std::vector<std::string> package_contents(const std::string &package);

  static bool has_package(const std::string &package);
};

#define REGISTER_OBJECT(M,O) shcore::Object_bridge_register<O>M ## _ ## O ## _register(#M,#O);
#define REGISTER_ALIASED_OBJECT(M,O,C) shcore::Object_bridge_register<C>M ## _ ## O ## _register(#M,#O);

template<class ObjectBridgeClass>
struct Object_bridge_register {
  Object_bridge_register(const std::string &module_name, const std::string &object_name) {
    shcore::Object_factory::register_factory(module_name, object_name, &ObjectBridgeClass::create);
  }
};
};
#endif //_OBJECT_FACTORY_H_

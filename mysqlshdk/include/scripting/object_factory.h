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
#ifndef _OBJECT_FACTORY_H_
#define _OBJECT_FACTORY_H_

#include "scripting/types.h"
#include "scripting/types_common.h"
#include "scripting/types_cpp.h"  // NamingStyle

namespace shcore {
class SHCORE_PUBLIC Object_factory {
 public:
  typedef std::shared_ptr<Object_bridge> (*Factory_function)(
      const Argument_list &args);

  //! Registers a metaclass
  static void register_factory(const std::string &package,
                               const std::string &class_name,
                               Factory_function function);

  static std::shared_ptr<Object_bridge> call_constructor(
      const std::string &package, const std::string &name,
      const Argument_list &args);

  static std::vector<std::string> package_names();
  static std::vector<std::string> package_contents(const std::string &package);

  static bool has_package(const std::string &package);
};

#define REGISTER_OBJECT(M, O) \
  shcore::Object_bridge_register<O> M##_##O##_register(#M, #O);
#define REGISTER_ALIASED_OBJECT(M, O, C) \
  shcore::Object_bridge_register<C> M##_##O##_register(#M, #O);

template <class ObjectBridgeClass>
struct Object_bridge_register {
  Object_bridge_register(const std::string &module_name,
                         const std::string &object_name) {
    shcore::Object_factory::register_factory(module_name, object_name,
                                             &ObjectBridgeClass::create);
  }
};
}  // namespace shcore
#endif  //_OBJECT_FACTORY_H_

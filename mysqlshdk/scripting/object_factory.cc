/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "scripting/object_factory.h"

using namespace shcore;

typedef std::map<std::string, Object_factory::Factory_function> Package;

static std::map<std::string, Package> *Object_Factories = NULL;

// --

void Object_factory::register_factory(const std::string &package,
                                      const std::string &class_name,
                                      Factory_function function)

{
  if (Object_Factories == NULL)
    Object_Factories = new std::map<std::string, Package>();

  Package &pkg = (*Object_Factories)[package];
  if (pkg.find(class_name) != pkg.end())
    throw std::logic_error("Registering duplicate Object Factory " + package +
                           "::" + class_name);
  pkg[class_name] = function;
}

std::shared_ptr<Object_bridge> Object_factory::call_constructor(
    const std::string &package, const std::string &name,
    const Argument_list &args, NamingStyle style) {
  std::map<std::string, Package>::iterator iter;
  Package::iterator piter;
  if ((iter = Object_Factories->find(package)) != Object_Factories->end() &&
      (piter = iter->second.find(name)) != iter->second.end()) {
    return piter->second(args, style);
  }
  throw std::invalid_argument("Invalid factory constructor " + package + "." +
                              name + " invoked");
}

std::vector<std::string> Object_factory::package_names() {
  std::vector<std::string> names;

  for (std::map<std::string, Package>::const_iterator iter =
           Object_Factories->begin();
       iter != Object_Factories->end(); ++iter)
    names.push_back(iter->first);
  return names;
}

bool Object_factory::has_package(const std::string &package) {
  return Object_Factories->find(package) != Object_Factories->end();
}

std::vector<std::string> Object_factory::package_contents(
    const std::string &package) {
  std::vector<std::string> names;

  std::map<std::string, Package>::iterator iter;
  if ((iter = Object_Factories->find(package)) != Object_Factories->end()) {
    for (Package::iterator i = iter->second.begin(); i != iter->second.end();
         ++i)
      names.push_back(i->first);
  }
  return names;
}

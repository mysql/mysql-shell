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

#include "shellcore/object_factory.h"

using namespace shcore;


typedef std::map<std::string, Object_factory*> Package;

static std::map<std::string, Package> *Object_Factories = NULL;

// --

void Object_factory::register_factory(const std::string &package, Object_factory *meta)
{
  if (Object_Factories == NULL)
    Object_Factories = new std::map<std::string, Package>();

  Package &pkg = (*Object_Factories)[package];
  if (pkg.find(meta->name()) != pkg.end())
    throw std::logic_error("Registering duplicate Object Factory "+package+"::"+meta->name());
  pkg[meta->name()] = meta;
}


boost::shared_ptr<Object_bridge> Object_factory::call_constructor(const std::string &package, const std::string &name,
                                                                  const Argument_list &args)
{
  std::map<std::string, Package>::iterator iter;
  Package::iterator piter;
  if ((iter = Object_Factories->find(package)) != Object_Factories->end()
      && (piter = iter->second.find(name)) != iter->second.end())
  {
    return boost::shared_ptr<Object_bridge>(piter->second->construct(args));
  }
  throw std::invalid_argument("Invalid factory constructor "+package+"."+name+" invoked");
}


std::vector<std::string> Object_factory::package_names()
{
  std::vector<std::string> names;

  for (std::map<std::string, Package>::const_iterator iter = Object_Factories->begin();
       iter != Object_Factories->end(); ++iter)
    names.push_back(iter->first);
  return names;
}


bool Object_factory::has_package(const std::string &package)
{
  return Object_Factories->find(package) != Object_Factories->end();
}


std::vector<std::string> Object_factory::package_contents(const std::string &package)
{
  std::vector<std::string> names;

  std::map<std::string, Package>::iterator iter;
  if ((iter = Object_Factories->find(package)) != Object_Factories->end())
  {
    for (Package::iterator i = iter->second.begin(); i != iter->second.end(); ++i)
      names.push_back(i->first);
  }
  return names;
}
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

#ifndef MODULES_ADMINAPI_MOD_DBA_INSTANCE_H_
#define MODULES_ADMINAPI_MOD_DBA_INSTANCE_H_

#include <string>
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"

namespace mysh {
namespace dba {

/**
* $(INSTANCE_BRIEF)
* 
* $(INSTANCE_DETAIL)
*/
class Instance : public shcore::Cpp_object_bridge{
 public:
  explicit Instance(const std::string &name, const std::string& uri, const shcore::Value::Map_type_ref options  = nullptr);
  virtual ~Instance(){};

  virtual std::string class_name() const { return "Instance"; }
  virtual bool operator == (const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;
  void set_name(std::string name) { _name = name; }
  
  // Accessors for C++
  std::string get_name() { return _name; }
  std::string get_uri() { return _uri; }
  std::string get_password() { return _password; }


#if DOXYGEN_JS
  String name;
  String uri;
  String options;
  String getName();
  String getUri();
  Dictionary getOptions();
/*
  Undefined start();
  Undefined stop();
  Undefined restart();
  Undefined kill();
  Undefined delete();
*/
#elif DOXYGEN_PY
  str name;
  str uri;
  str get_name();
  str get_uri();
  dict get_options();
/*
  None start();
  None stop();
  None restart();
  None kill();
  None delete();
*/
#endif

/*
  shcore::Value start(const shcore::Argument_list &args);
  shcore::Value stop(const shcore::Argument_list &args);
  shcore::Value restart(const shcore::Argument_list &args);
  shcore::Value kill(const shcore::Argument_list &args);
  shcore::Value delete(const shcore::Argument_list &args);
*/

 private:
  void init();

 protected:
  std::string _name;
  std::string _uri;
  
  // Caching the instance password
  std::string _password;
  shcore::Value::Map_type_ref _options;
};
}  // namespace dba
}  // namespace mysh

#endif  // MODULES_ADMINAPI_MOD_DBA_INSTANCE_H_
/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_ADMINAPI_MOD_DBA_INSTANCE_H_
#define MODULES_ADMINAPI_MOD_DBA_INSTANCE_H_

#include <string>
#include "scripting/types.h"
#include "scripting/types_cpp.h"

namespace mysqlsh {
namespace dba {

/**
 * $(INSTANCE_BRIEF)
 *
 * $(INSTANCE_DETAIL)
 */
class Instance : public shcore::Cpp_object_bridge {
 public:
  explicit Instance(const std::string &name, const std::string &uri,
                    const shcore::Value::Map_type_ref options = nullptr);
  virtual ~Instance() {}

  virtual std::string class_name() const { return "Instance"; }
  virtual bool operator==(const Object_bridge &other) const;

  virtual shcore::Value get_member(const std::string &prop) const;
  void set_name(std::string name) { _name = name; }

  // Accessors for C++
  std::string get_name() { return _name; }
  std::string get_uri() { return _uri; }
  std::string get_password() { return _password; }

#if DOXYGEN_JS
  String name;              //!< $(INSTANCE_NAME_BRIEF)
  String uri;               //!< $(INSTANCE_URI_BRIEF)
  Dictionary options;       //!< $(INSTANCE_OPTIONS_BRIEF)
  String getName();         //!< $(INSTANCE_GETNAME_BRIEF)
  String getUri();          //!< $(INSTANCE_GETURI_BRIEF)
  Dictionary getOptions();  //!< $(INSTANCE_GETOPTIONS_BRIEF)
/*
  Undefined start();
  Undefined stop();
  Undefined restart();
  Undefined kill();
  Undefined delete();
*/
#elif DOXYGEN_PY
  str name;            //!< $(INSTANCE_NAME_BRIEF)
  str uri;             //!< $(INSTANCE_URI_BRIEF)
  dict options;        //!< $(INSTANCE_OPTIONS_BRIEF)
  str get_name();      //!< $(INSTANCE_GETNAME_BRIEF)
  str get_uri();       //!< $(INSTANCE_GETURI_BRIEF)
  dict get_options();  //!< $(INSTANCE_GETOPTIONS_BRIEF)
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
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_INSTANCE_H_

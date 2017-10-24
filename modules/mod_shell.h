/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_MOD_SHELL_H_
#define MODULES_MOD_SHELL_H_

#include <memory>
#include <string>
#include "modules/mod_shell_options.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "scripting/types_cpp.h"
#include "shellcore/base_session.h"
#include "src/mysqlsh/mysql_shell.h"

namespace mysqlsh {
/**
 * \ingroup ShellAPI
 * $(SHELL_BRIEF)
 */
class SHCORE_PUBLIC Shell : public shcore::Cpp_object_bridge
                            DEBUG_OBJ_FOR_CLASS_(Shell) {
 public:
  Shell(mysqlsh::Mysql_shell *owner);
  virtual ~Shell();

  virtual std::string class_name() const {
    return "Shell";
  }
  virtual bool operator==(const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

  shcore::Value parse_uri(const shcore::Argument_list &args);
  shcore::Value prompt(const shcore::Argument_list &args);
  shcore::Value connect(const shcore::Argument_list &args);

  void set_current_schema(const std::string &name);

  shcore::Value _set_current_schema(const shcore::Argument_list &args);
  shcore::Value set_session(const shcore::Argument_list &args);
  shcore::Value get_session(const shcore::Argument_list &args);
  shcore::Value reconnect(const shcore::Argument_list &args);
  shcore::Value log(const shcore::Argument_list &args);

#if DOXYGEN_JS
  Dictionary options;
  Dictionary parseUri(String uri);
  String prompt(String message, Dictionary options);
  Undefined connect(ConnectionData connectionData, String password);
  Undefined log(String level, String message);
#elif DOXYGEN_PY
  dict options;
  dict parse_uri(str uri);
  str prompt(str message, dict options);
  None connect(ConnectionData connectionData, str password);
  None log(str level, str message);
#endif

  std::shared_ptr<mysqlsh::ShellBaseSession> set_session_global(
      const std::shared_ptr<mysqlsh::ShellBaseSession> &session);
  std::shared_ptr<mysqlsh::ShellBaseSession> get_dev_session();

 protected:
  void init();

  mysqlsh::Mysql_shell *_shell;
  shcore::IShell_core *_shell_core;
  std::shared_ptr<shcore::Mod_shell_options> _core_options;
};
}  // namespace mysqlsh

#endif  // MODULES_MOD_SHELL_H_

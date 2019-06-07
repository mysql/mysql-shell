/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_MOD_SHELL_H_
#define MODULES_MOD_SHELL_H_

#include <memory>
#include <string>
#include "modules/devapi/base_resultset.h"
#include "modules/mod_extensible_object.h"
#include "modules/mod_shell_options.h"
#include "modules/mod_shell_reports.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "scripting/types_cpp.h"
#include "shellcore/base_session.h"
#include "src/mysqlsh/mysql_shell.h"

namespace mysqlsh {
#if DOXYGEN_JS
Array dir(Object object);
#endif

/**
 * \ingroup ShellAPI
 *
 * $(SHELL_BRIEF)
 */
class SHCORE_PUBLIC Shell : public shcore::Cpp_object_bridge
                            DEBUG_OBJ_FOR_CLASS_(Shell) {
 public:
  Shell(mysqlsh::Mysql_shell *owner);
  virtual ~Shell();

  virtual std::string class_name() const { return "Shell"; }
  virtual bool operator==(const Object_bridge &other) const;

  virtual void set_member(const std::string &prop, shcore::Value value);
  virtual shcore::Value get_member(const std::string &prop) const;

  shcore::Dictionary_t parse_uri(const std::string &uri);
  std::string unparse_uri(const shcore::Dictionary_t &options);

  shcore::Value prompt(const shcore::Argument_list &args);
  shcore::Value connect(const shcore::Argument_list &args);

#if !defined(DOXYGEN_PY)
  void set_current_schema(const std::string &name);
#endif

  shcore::Value _set_current_schema(const shcore::Argument_list &args);
  shcore::Value set_session(const shcore::Argument_list &args);
  shcore::Value get_session(const shcore::Argument_list &args);
  shcore::Value reconnect(const shcore::Argument_list &args);
  shcore::Value log(const shcore::Argument_list &args);
  shcore::Value status(const shcore::Argument_list &args);

#if DOXYGEN_JS
  Options options;
  Reports reports;
  Dictionary parseUri(String uri);
  String unparseUri(Dictionary options);
  String prompt(String message, Dictionary options);
  Undefined connect(ConnectionData connectionData, String password);
  Session getSession();
  Undefined setSession(Session session);
  Undefined setCurrentSchema(String name);
  Undefined log(String level, String message);
  Undefined reconnect();
  Undefined status();
  List listCredentialHelpers();
  Undefined storeCredential(String url, String password);
  Undefined deleteCredential(String url);
  Undefined deleteAllCredentials();
  List listCredentials();
  Undefined enablePager();
  Undefined disablePager();
  Undefined registerReport(String name, String type, Function report,
                           Dictionary description);
  UserObject createExtensionObject();
  Undefined addExtensionObjectMember(Object object, String name, Value member,
                                     Dictionary definition);
  Undefined registerGlobal(String name, Object object, Dictionary definition);
  Integer dumpRows(ShellBaseResult result, String format);
#elif DOXYGEN_PY
  Options options;
  Reports reports;
  dict parse_uri(str uri);
  str unparse_uri(dict options);
  str prompt(str message, dict options);
  None connect(ConnectionData connectionData, str password);
  Session get_session();
  None set_session(Session session);
  None set_current_schema(str name);
  None log(str level, str message);
  None reconnect();
  None status();
  list list_credential_helpers();
  None store_credential(str url, str password);
  None delete_credential(str url);
  None delete_all_credentials();
  list list_credentials();
  None enable_pager();
  None disable_pager();
  None register_report(str name, str type, Function report, dict description);
  UserObject create_extension_object();
  Undefined add_extension_object_member(Object object, str name, Value member,
                                        dict definition);
  Undefined register_global(str name, Object object, dict definition);
  int dump_rows(ShellBaseResult result, str format);
#endif

  shcore::Value list_credential_helpers(const shcore::Argument_list &args);
  shcore::Value store_credential(const shcore::Argument_list &args);
  shcore::Value delete_credential(const shcore::Argument_list &args);
  shcore::Value delete_all_credentials(const shcore::Argument_list &args);
  shcore::Value list_credentials(const shcore::Argument_list &args);

  std::shared_ptr<mysqlsh::ShellBaseSession> set_session_global(
      const std::shared_ptr<mysqlsh::ShellBaseSession> &session);
  std::shared_ptr<mysqlsh::ShellBaseSession> get_dev_session();
  std::shared_ptr<mysqlsh::Options> get_shell_options() {
    return _core_options;
  }
  std::shared_ptr<mysqlsh::Shell_reports> get_shell_reports() {
    return m_reports;
  }

  void enable_pager();
  void disable_pager();

  void register_report(const std::string &name, const std::string &type,
                       const shcore::Function_base_ref &report,
                       const shcore::Dictionary_t &options);

  std::shared_ptr<Extensible_object> create_extension_object();
  void add_extension_object_member(std::shared_ptr<Extensible_object> object,
                                   const std::string &name,
                                   const shcore::Value &member,
                                   const shcore::Dictionary_t &definition = {});
  void register_global(const std::string &name,
                       std::shared_ptr<Extensible_object> object,
                       const shcore::Dictionary_t &definition = {});

  int dump_rows(const std::shared_ptr<ShellBaseResult> &resultset,
                const std::string &format);

 protected:
  void init();

  mysqlsh::Mysql_shell *_shell;
  shcore::IShell_core *_shell_core;
  std::shared_ptr<mysqlsh::Options> _core_options;

 private:
  std::shared_ptr<mysqlsh::Shell_reports> m_reports;
};
}  // namespace mysqlsh

#endif  // MODULES_MOD_SHELL_H_

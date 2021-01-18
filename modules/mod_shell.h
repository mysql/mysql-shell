/*
 * Copyright (c) 2016, 2021, Oracle and/or its affiliates.
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
#include "modules/mod_shell_context.h"
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
Any require(String module_name_or_path);
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

  std::string prompt(const std::string &message,
                     const shcore::Dictionary_t &options = {});
  std::shared_ptr<ShellBaseSession> connect(
      const mysqlshdk::db::Connection_options &connection_options,
      const char *password = {});
  std::shared_ptr<ShellBaseSession> connect_to_primary(
      const mysqlshdk::db::Connection_options &connection_options =
          mysqlshdk::db::Connection_options{},
      const char *password = {});
  std::shared_ptr<ShellBaseSession> open_session(
      const mysqlshdk::db::Connection_options &connection_options,
      const char *password = {});

#if !defined(DOXYGEN_PY)
  void set_current_schema(const std::string &name);
  void set_session(const std::shared_ptr<ShellBaseSession> &session);
#endif
  std::shared_ptr<ShellBaseSession> get_session();
  void disconnect();
  bool reconnect();
  void log(const std::string &level, const std::string &message);
  void status();

#if DOXYGEN_JS
  Options options;
  Reports reports;
  String version;
  Dictionary parseUri(String uri);
  String unparseUri(Dictionary options);
  String prompt(String message, Dictionary options);
  Session connect(ConnectionData connectionData, String password);
  Session connectToPrimary(ConnectionData connectionData, String password);
  Session openSession(ConnectionData connectionData, String password);
  Session getSession();
  Undefined setSession(Session session);
  Undefined setCurrentSchema(String name);
  Undefined log(String level, String message);
  Undefined disconnect();
  Bool reconnect();
  Undefined status();
  List listCredentialHelpers();
  Undefined storeCredential(String url, String password);
  Undefined deleteCredential(String url);
  Undefined deleteAllCredentials();
  List listCredentials();
  List listSshConnections();
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
  str version;
  dict parse_uri(str uri);
  str unparse_uri(dict options);
  str prompt(str message, dict options);
  Session connect(ConnectionData connectionData, str password);
  Session connect_to_primary(ConnectionData connectionData, str password);
  Session open_session(ConnectionData connectionData, str password);
  Session get_session();
  None set_session(Session session);
  None set_current_schema(str name);
  None log(str level, str message);
  None disconnect();
  bool reconnect();
  None status();
  list list_credential_helpers();
  None store_credential(str url, str password);
  None delete_credential(str url);
  None delete_all_credentials();
  list list_credentials();
  ShellContextWrapper create_context(dict options);
  list list_ssh_connections();
  None enable_pager();
  None disable_pager();
  None register_report(str name, str type, Function report, dict description);
  UserObject create_extension_object();
  Undefined add_extension_object_member(Object object, str name, Value member,
                                        dict definition);
  Undefined register_global(str name, Object object, dict definition);
  int dump_rows(ShellBaseResult result, str format);
#endif

  shcore::Array_t list_credential_helpers();
  void store_credential(
      const std::string &url,
      const mysqlshdk::utils::nullable<std::string> &password);
  void delete_credential(const std::string &url);
  void delete_all_credentials();
  shcore::Array_t list_credentials();

  shcore::Array_t list_ssh_connections();

  std::shared_ptr<mysqlsh::ShellBaseSession> set_session_global(
      const std::shared_ptr<mysqlsh::ShellBaseSession> &session);
  std::shared_ptr<mysqlsh::ShellBaseSession> get_dev_session();
  std::shared_ptr<mysqlsh::Options> get_shell_options() {
    return _core_options;
  }
  std::shared_ptr<mysqlsh::Shell_reports> get_shell_reports() {
    return m_reports;
  }

  std::shared_ptr<Shell_context_wrapper> create_context(
      const shcore::Option_pack_ref<Shell_context_wrapper_options> &callbacks);

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

  shcore::Dictionary_t get_globals(shcore::IShell_core::Mode mode) const;

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

/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MODULES_ADMINAPI_BASE_CLUSTER_H_
#define MODULES_ADMINAPI_BASE_CLUSTER_H_

#include <string>

#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/mod_dba_routing_guideline.h"
#include "modules/mod_shell_result.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
#include "scripting/types.h"
#include "scripting/types_cpp.h"

namespace mysqlsh {
namespace dba {

class Base_cluster : public shcore::Cpp_object_bridge {
 public:
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  void append_json(shcore::JSON_dumper &dumper) const override;
  bool operator==(const Object_bridge &other) const override;

  shcore::Value get_member(const std::string &prop) const override;

 public:  // Router management
  shcore::Dictionary_t list_routers(
      const shcore::Option_pack_ref<List_routers_options> &options);

  void set_routing_option(const std::string &option,
                          const shcore::Value &value);

  void set_routing_option(const std::string &router, const std::string &option,
                          const shcore::Value &value);

  shcore::Dictionary_t routing_options(const std::string &router);

  shcore::Dictionary_t router_options(
      const shcore::Option_pack_ref<Router_options_options> &options);

  shcore::Value execute(
      const std::string &cmd, const shcore::Value &instances,
      const shcore::Option_pack_ref<Execute_options> &options);

 public:  // Routing Guidelines
  std::shared_ptr<RoutingGuideline> create_routing_guideline(
      const std::string &name, shcore::Dictionary_t json,
      const shcore::Option_pack_ref<Create_routing_guideline_options> &options);

  std::shared_ptr<RoutingGuideline> get_routing_guideline(
      const std::string &name) const;

  void remove_routing_guideline(const std::string &name);

  std::shared_ptr<ShellResult> routing_guidelines() const;

  std::shared_ptr<RoutingGuideline> import_routing_guideline(
      const std::string &file_path,
      const shcore::Option_pack_ref<Import_routing_guideline_options> &options);

 public:  // User management
  void setup_admin_account(
      const std::string &user,
      const shcore::Option_pack_ref<Setup_account_options> &options);

  void setup_router_account(
      const std::string &user,
      const shcore::Option_pack_ref<Setup_account_options> &options);

 protected:
  bool m_invalidated = false;

  virtual std::shared_ptr<Base_cluster_impl> base_impl() const = 0;

  virtual void assert_valid(const std::string &option_name) const = 0;
  void invalidate() { m_invalidated = true; }

  template <typename TCallback>
  auto execute_with_pool(TCallback &&f, bool interactive) const {
    // Invalidate the cached metadata state
    base_impl()->get_metadata_storage()->invalidate_cached();

    // Init the connection pool
    Scoped_instance_pool ipool(
        interactive,
        Instance_pool::Auth_options(base_impl()->default_admin_credentials()));

    return f();
  }
};

REGISTER_HELP_SHARED_TEXT(LISTROUTERS_HELP_TEXT, (R"*(
Lists the Router instances.

@param options Optional dictionary with options for the operation.

@returns A JSON object listing the Router instances associated to the <<<:Type>>>.

This function lists and provides information about all Router instances
registered for the <<<:Type>>>.

For each router, the following information is provided, when available:

@li hostname: Hostname.
@li lastCheckIn: Timestamp of the last statistics update (check-in).
@li roPort: Read-only port (Classic protocol).
@li roXPort: Read-only port (X protocol).
@li rwPort: Read-write port (Classic protocol).
@li rwSplitPort: Read-write split port (Classic protocol).
@li rwXPort: Read-write port (X protocol).
@li upgradeRequired: If true, it indicates Router is incompatible with the
Cluster's metadata version and must be upgraded.
@li version: Version.

Whenever a Metadata Schema upgrade is necessary, the recommended process
is to upgrade MySQL Router instances to the latest version before upgrading
the Metadata itself, in order to minimize service disruption.

The options dictionary may contain the following attributes:

@li onlyUpgradeRequired: boolean, enables filtering so only router instances
that support older version of the Metadata Schema and require upgrade are
included.
)*"));

REGISTER_HELP_SHARED_TEXT(SETUPADMINACCOUNT_HELP_TEXT, (R"*(
Create or upgrade an <<<:FullType>>> admin account.

@param user Name of the <<<:FullType>>> administrator account.
@param options Dictionary with options for the operation.

@returns Nothing.

This function creates/upgrades a MySQL user account with the necessary
privileges to administer an <<<:FullType>>>.

This function also allows a user to upgrade an existing admin account
with the necessary privileges before a dba.<<<upgradeMetadata>>>() call.

The mandatory argument user is the name of the MySQL account we want to create
or upgrade to be used as Administrator account. The accepted format is
username[@@host] where the host part is optional and if not provided defaults to
'%'.

The options dictionary may contain the following attributes:

@li password: The password for the <<<:FullType>>> administrator account.
@li passwordExpiration: Password expiration setting for the account. May be set
to the number of days for expiration, 'NEVER' to disable expiration and 'DEFAULT'
to use the system default.
@li requireCertIssuer: Optional SSL certificate issuer for the account.
@li requireCertSubject: Optional SSL certificate subject for the account.
@li dryRun: boolean value used to enable a dry run of the account setup
process. Default value is False.
@li update: boolean value that must be enabled to allow updating the privileges and/or
password of existing accounts. Default value is False.

If the user account does not exist, either the password, requireCertIssuer or
requireCertSubject are mandatory.

If the user account exists, the update option must be enabled.

If dryRun is used, the function will display information about the permissions to
be granted to `user` account without actually creating and/or performing any
changes to it.

To change authentication options for an existing account, set `update` to `true`.
It is possible to change password without affecting certificate options or
vice-versa but certificate options can only be changed together.
)*"));

REGISTER_HELP_SHARED_TEXT(SETUPROUTERACCOUNT_HELP_TEXT, (R"*(
Create or upgrade a MySQL account to use with MySQL Router.

@param user Name of the account to create/upgrade for MySQL Router.
@param options Dictionary with options for the operation.

@returns Nothing.

This function creates/upgrades a MySQL user account with the necessary
privileges to be used by MySQL Router.

This function also allows a user to upgrade existing MySQL router accounts
with the necessary privileges after a dba.<<<upgradeMetadata>>>() call.

The mandatory argument user is the name of the MySQL account we want to create
or upgrade to be used by MySQL Router. The accepted format is
username[@@host] where the host part is optional and if not provided defaults to
'%'.

The options dictionary may contain the following attributes:

@li password: The password for the MySQL Router account.
@li passwordExpiration: Password expiration setting for the account. May be set
to the number of days for expiration, 'NEVER' to disable expiration and 'DEFAULT'
to use the system default.
@li requireCertIssuer: Optional SSL certificate issuer for the account.
@li requireCertSubject: Optional SSL certificate subject for the account.
@li dryRun: boolean value used to enable a dry run of the account setup
process. Default value is False.
@li update: boolean value that must be enabled to allow updating the privileges and/or
password of existing accounts. Default value is False.

If the user account does not exist, either the password, requireCertIssuer or
requireCertSubject are mandatory.

If the user account exists, the update option must be enabled.

If dryRun is used, the function will display information about the permissions to
be granted to `user` account without actually creating and/or performing any
changes to it.

To change authentication options for an existing account, set `update` to `true`.
It is possible to change password without affecting certificate options or
vice-versa but certificate options can only be changed together.
)*"));

REGISTER_HELP_SHARED_TEXT(ROUTINGOPTIONS_HELP_TEXT, (R"*(
Lists the <<<:Type>>> Routers configuration options.

@param router Optional identifier of the router instance to query for the options.

@returns A JSON object describing the configuration options of all router
instances of the <<<:Type>>> and its global options or just the given Router.

@attention This function is deprecated and will be removed in a future release
of MySQL Shell. Use <<<:Type>>>.routerOptions() instead.

This function lists the Router configuration options of all Routers of the
<<<:Type>>> or the target Router.
)*"));

REGISTER_HELP_SHARED_TEXT(CREATEROUTINGGUIDELINE_HELP_TEXT, (R"*(
Creates a new Routing Guideline for the <<<:Type>>>.

@param name The identifier name for the new Routing Guideline.
@param json Optional JSON document defining the Routing Guideline content.
@param options Optional dictionary with options for the operation.

@returns A RoutingGuideline object representing the newly created Routing
Guideline.

This command creates a new Routing Guideline that defines MySQL Router's
routing behavior using rules that specify potential destination MySQL servers
for incoming client sessions.

You can optionally pass a JSON document defining the Routing Guideline via the
'json' parameter. This must be a valid Routing Guideline definition, with the
exception of the "name" field, which is overridden by the provided 'name'
parameter.

If the 'json' parameter is not provided, a default Routing Guideline is created
based on the parent topology type. The guideline is automatically populated
with default values tailored to the topology, ensuring the Router's
default behavior for that topology is represented.

The newly created guideline won't be set as the active guideline for the
topology. That needs to be explicitly done with <<<:Type>>>.setRoutingOption()
using the option 'guideline'.

The following options are supported:

@li force (boolean): Allows overwriting an existing Routing Guideline with the
specified name. Disabled by default.

<b>Behavior</b>

@li If 'json' is not provided, a default Routing Guideline is created according
to the parent topology type.
@li If 'json' is provided, the content from the JSON is used, except for the
"name" field, which is overridden by the 'name' parameter.

For more information on Routing Guidelines, see \\? RoutingGuideline.
)*"));

REGISTER_HELP_SHARED_TEXT(GETROUTINGGUIDELINE_HELP_TEXT, (R"*(
Returns the named Routing Guideline.

@param name Optional name of the Guideline to be returned.

@returns The Routing Guideline object.

Returns the named Routing Guideline object associated to the <<<:Type>>>. If no
name is given, the guideline currently active for the <<<:Type>>> is returned.
If there is none, then an exception is thrown.

For more information about Routing Guidelines, see \\? RoutingGuideline
)*"));

REGISTER_HELP_SHARED_TEXT(REMOVEROUTINGGUIDELINE_HELP_TEXT, (R"*(
Removes the named Routing Guideline.

@param name Name of the Guideline to be removed.

@returns Nothing.

Removes the named Routing Guideline object associated to the <<<:Type>>>.

For more information about Routing Guidelines, see \\? RoutingGuideline
)*"));

REGISTER_HELP_SHARED_TEXT(ROUTINGGUIDELINES_HELP_TEXT, (R"*(
Lists the Routing Guidelines defined for the <<<:Type>>>.

@returns The list of Routing Guidelines of the <<<:Type>>>.

For more information about Routing Guidelines, see \\? RoutingGuideline
)*"));

REGISTER_HELP_SHARED_TEXT(IMPORTROUTINGGUIDELINE_HELP_TEXT, (R"*(
Imports a Routing Guideline from a JSON file into the <<<:Type>>>.

@param file The file path to the JSON file containing the Routing Guideline.
@param options Optional dictionary with options for the operation.

@returns A RoutingGuideline object representing the newly imported Routing
Guideline.

This command imports a Routing Guideline from a JSON file into the
target topology. The imported guideline will be validated before it is saved
in the topology's metadata.

The imported guideline won't be set as the active guideline for the
topology. That needs to be explicitly done with <<<:Type>>>.setRoutingOption()
using the option 'guideline'.

The following options are supported:

@li force (boolean): Allows overwriting an existing Routing Guideline with the
same name. Disabled by default.

For more information on Routing Guidelines, see \\? RoutingGuideline.
)*"));

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_BASE_CLUSTER_H_

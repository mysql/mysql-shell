/*
 * Copyright (c) 2022, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_BASE_CLUSTER_H_
#define MODULES_ADMINAPI_BASE_CLUSTER_H_

#include <memory>
#include <string>

#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/base_cluster_impl.h"
#include "modules/adminapi/common/common.h"
#include "mysqlshdk/include/shellcore/utils_help.h"
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

 public:  // User management
  void setup_admin_account(
      const std::string &user,
      const shcore::Option_pack_ref<Setup_account_options> &options);

  void setup_router_account(
      const std::string &user,
      const shcore::Option_pack_ref<Setup_account_options> &options);

 protected:
  bool m_invalidated = false;

  virtual Base_cluster_impl *base_impl() const = 0;

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
${OPT_SETUP_ACCOUNT_OPTIONS_PASSWORD_EXPIRATION}
${OPT_SETUP_ACCOUNT_OPTIONS_REQUIRE_CERT_ISSUER}
${OPT_SETUP_ACCOUNT_OPTIONS_REQUIRE_CERT_SUBJECT}
${OPT_SETUP_ACCOUNT_OPTIONS_DRY_RUN}
${OPT_INTERACTIVE}
${OPT_SETUP_ACCOUNT_OPTIONS_UPDATE}

If the user account does not exist, either the password, requireCertIssuer or
requireCertSubject are mandatory.

If the user account exists, the update option must be enabled.

${OPT_SETUP_ACCOUNT_OPTIONS_DRY_RUN_DETAIL}

The interactive option can be used to explicitly enable or disable the
interactive prompts that help the user through the account setup process.

${OPT_SETUP_ACCOUNT_OPTIONS_UPDATE_DETAIL}

@attention The interactive option will be removed in a future release.
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
${OPT_SETUP_ACCOUNT_OPTIONS_PASSWORD_EXPIRATION}
${OPT_SETUP_ACCOUNT_OPTIONS_REQUIRE_CERT_ISSUER}
${OPT_SETUP_ACCOUNT_OPTIONS_REQUIRE_CERT_SUBJECT}
${OPT_SETUP_ACCOUNT_OPTIONS_DRY_RUN}
${OPT_INTERACTIVE}
${OPT_SETUP_ACCOUNT_OPTIONS_UPDATE}

If the user account does not exist, either the password, requireCertIssuer or
requireCertSubject are mandatory.

If the user account exists, the update option must be enabled.

${OPT_SETUP_ACCOUNT_OPTIONS_DRY_RUN_DETAIL}

${OPT_SETUP_ACCOUNT_OPTIONS_INTERACTIVE_DETAIL}

${OPT_SETUP_ACCOUNT_OPTIONS_UPDATE_DETAIL}

@attention The interactive option will be removed in a future release.
)*"));

REGISTER_HELP_SHARED_TEXT(ROUTINGOPTIONS_HELP_TEXT, (R"*(
Lists the <<<:Type>>> Routers configuration options.

@param router Optional identifier of the router instance to query for the options.

@returns A JSON object describing the configuration options of all router
instances of the <<<:Type>>> and its global options or just the given Router.

This function lists the Router configuration options of all Routers of the
<<<:Type>>> or the target Router.
)*"));

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_BASE_CLUSTER_H_

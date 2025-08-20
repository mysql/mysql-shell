# Copyright (c) 2025, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

from mysqlsh.plugin_manager import plugin, plugin_function
from mysqlsh import globals, Error
from mysqlsh.mysql import split_account

import util.lib.common as common
import util.lib.account as accountlib

def prompt_confirmed_password(newPassword, is_new: bool, custom_description: str = None) -> str:
    if newPassword is not None:
        return newPassword
    
    is_new = "new " if is_new else ""

    description = [custom_description] if custom_description else []
    for _ in range(3):
        new_password = globals.shell.prompt(
            f"Enter {is_new}password", {"type": "password", "description": description})
        confirm = globals.shell.prompt(
            f"Confirm {is_new}password", {"type": "password"})
        if new_password == confirm:
            return new_password
        description = ["Passwords don't match."]

    raise Error("Failed to enter matching passwords.")


def check_mysql_native_method(session, account_data):
    if not accountlib.is_auth_mysql_native(session, account_data):
        globals.shell.print(
            f"Account {account_data['account']} is not using \"mysql_native_password\". No changes were made.", "note")
        return False
    return True


def discard_dual_if_present(discardOld: bool, account_data: dict):
    if discardOld:
        if not accountlib.is_password_extensions_supported(globals.shell.get_session()):
            raise Error("Dual password functionality is only supported from server version at least 8.0.")
        try:
            if accountlib.discard_dual_password(account_data):
                globals.shell.print("Old (dual) password was succesfully discarded.", "note")
            else:
                globals.shell.print("Account " + account_data["account"] +
                      " does not have a dual password present.", "note")
        except Error as e:
            globals.shell.print(f"Failed to discard old password: {e.msg}", "error")
        return False
    return True


def check_dual_password(session, account_data: dict, dual: bool):
    if dual and not accountlib.is_password_extensions_supported(session):
        raise Error("Dual password functionality is only supported from server version at least 8.0.")
    has_dual = False
    try:
        has_dual = accountlib.has_dual_password(session, account_data)
    except Error as e:
        globals.shell.log(
                "WARNING", f"Could not retrieve dual password status for {account_data['account']}: {e.msg}")
    
    if has_dual:
        message = f"The account {account_data["account"]} has a retained old (dual) password."
        type = "warning"
        suggestion = "It can be discarded"
        instruction = "using util.<<<changePassword>>>({\"account\":\"" + account_data["account"] + "\", \"discardOld\": True})."
        if dual:
            type = "error"
            suggestion = "Please discard it first"

        globals.shell.print(f"{message} {suggestion} {instruction}", type)

        if type == "error":
            raise Error("Unable to discard existing dual password implicitly")


def check_before_change_password(session, account_data: dict, dual: bool, random : bool):
    if accountlib.is_auth_mysql_native(session, account_data) and accountlib.is_server_auth_method_upgradable(session):
        globals.shell.print("The account " + account_data["account"] +
            " is using the deprecated mysql_native_password authentication plugin. "
            "Please use the \"util.<<<changeAuthMethod>>>\" function to upgrade it to caching_sha2_password.", "warning")

    check_dual_password(session, account_data, dual)

    if random and not accountlib.is_password_extensions_supported(session):
        raise Error("Random password functionality is only supported from server version at least 8.0.")


def internal_change_password(
        account: str = None,
        random: bool = False,
        dual: bool = False,
        discardOld: bool = False,
        newPassword: str = None
) -> str:
    if not globals.shell.get_session():
        print("Not connected!")
        return None

    session = globals.shell.get_session()

    account_data, account = common.collect_account_data(account)

    if not discard_dual_if_present(discardOld, account_data):
        return None

    print("Changing password for " + account + ".")

    check_before_change_password(session, account_data, dual, random)

    description = None
    for _ in range(3):
        if not random:
            newPassword = prompt_confirmed_password(
                newPassword, True, description)
        try:
            result = accountlib.change_password(
                session, account_data, random, dual, newPassword)

            if random:
                globals.shell.print(
                    "Password has been successfully updated to a random one.", "note")
                return result
            else:
                if dual:
                    globals.shell.print(
                        "Password changed successfully, the current password will remain valid (dual password enabled).", "note")
                    globals.shell.print(
                        "To discard the old password, use util.<<<changePassword>>>({\"account\":\"" + account + "\", \"discardOld\":True}).")
                else:
                    globals.shell.print(
                        "Password has been successfully updated.", "note")

            return None
        except Error as e:
            description = e.msg
            if e.code == 1819 and not random:  # password policy error code
                newPassword = None
                continue
            else:
                break

    raise Error(f"Failed to change password: {description}")


def internal_upgrade_auth_method(
        account: str = None,
        password: str = None
):

    session = globals.shell.get_session()

    if not session:
        print("Not connected!")
        return

    if not accountlib.is_server_auth_method_upgradable(session):
        raise Error("Unsupported server version. "
                    "To upgrade authentication method to caching_sha2_password, please upgrade the server to version at least 8.0.")

    account_data, _ = common.collect_account_data(account)

    if not check_mysql_native_method(session, account_data):
        return

    description = None
    for _ in range(3):
        password = prompt_confirmed_password(password, False, description)

        print(
            "Switching authentication plugin from mysql_native_password to caching_sha2_password for account " + account_data["account"] + ".")

        try:
            accountlib.upgrade_auth_method(session, account_data, password)

            globals.shell.print(
                "Authentication and password has been successfully updated.", "note")

            return
        except Error as e:
            description = e.msg
            if e.code == 1819:
                password = None
                continue
            else:
                break

    raise Error(f"Failed to change authentication method: {description}")


@plugin_function("util.changePassword")
def change_password(**options) -> str:
    """Changes password for an account.

    Changes the password of an account.
    If no account is specified, currently authenticated user is used.

    Examples:
        util.<<<changePassword>>>() - changes the password of current user.
        util.<<<changePassword>>>({"account":"user@localhost", "random":True}) - changes the password of "user@localhost" to a random one.
        util.<<<changePassword>>>({"newPassword":"xxxx", "dual":True"}) - changes password to newPassword without prompting, retains current as dual password.
        util.<<<changePassword>>>({"discardOld": True, "account": "other@localhost"}) - Discard retained password for account "other@localhost".

    Args:
        **options (dict): Optional arguments

    Keyword Args:
        account (str): account of wchich password will be changed. If not set, currently authenticated user will be used (default not set)
        random (bool): changes password to a random one and returns it (default False)
        dual (bool): retains current password after changing (default False)
        discardOld (bool):  discards retained old password if present. If True, will ignore other options (beside account) and not change current password. (default False)
        newPassword (str): password to change to for the specified account. If not set, it will be prompted (default not set)
    """
    return internal_change_password(**options)


@plugin_function("util.upgradeAuthMethod")
def upgrade_auth_method(**options):
    """Upgrades authentication plugin of an account.

    Checks and upgrades authentication method of an account from deprecated mysql_native_password to caching_sha2_password.
    If no account is specified, currently authenticated user is used.
    If targeted account does not use mysql_native_password as its authentication method, function exits.

    Args:
        **options (dict): Optional arguments

    Keyword Args:
        account (str): account to wchich upgrade will be performed; if not set, currently authenticated user will be used (default not set)
        password (str): password of currently authenticated user, that will be making changes; if not set, it will be prompted (default not set)
    """
    internal_upgrade_auth_method(**options)

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

from mysqlsh import globals, Error
from mysqlsh.mysql import make_account

def is_auth_mysql_native(session, account_data: dict) -> bool:
    account = make_account(account_data["user"], account_data["host"])
    statement = session.run_sql(
        f"SHOW CREATE USER {account};").fetch_one()[0]
    found_pos = statement.find("IDENTIFIED WITH")
    return statement.find("mysql_native_password", found_pos) > -1


def has_dual_password(session, account_data: dict):
    args = [account_data["user"], account_data["host"]]
    result = session.run_sql(
        "select JSON_CONTAINS_PATH(user_attributes, 'one', '$.additional_password') from mysql.user where user=? and host=?",
        args)
    info = result.fetch_one()
    return info[0] == 1


def discard_dual_password(account_data: dict):
    if not has_dual_password(globals.shell.get_session(), account_data):
        return False

    account = make_account(account_data["user"], account_data["host"])
    globals.shell.get_session().run_sql(
        f"ALTER USER {account} DISCARD OLD PASSWORD;")
    return True


def change_password(session, account_data: dict, random: bool, dual: bool, new_password: str):
    account = make_account(account_data["user"], account_data["host"])

    args = []
    sql_str = f"ALTER USER {account} IDENTIFIED BY"
    if random:
        sql_str += " RANDOM PASSWORD"
    else:
        sql_str += " ?"
        args.append(new_password)

    if dual:
        sql_str += " RETAIN CURRENT PASSWORD"

    result = session.run_sql(sql_str, args)

    if random:
        info = result.fetch_one()
        return info[2]  # third column is the generated password
    return None


def upgrade_auth_method(session, account_data: dict, new_password: str):
    account = make_account(account_data["user"], account_data["host"])
    args = [new_password]
    sql_str = f"ALTER USER {account} IDENTIFIED WITH 'caching_sha2_password' BY ?"
    session.run_sql(sql_str, args)


def get_server_major_version(session) -> int:
    result = session.run_sql("SELECT VERSION();").fetch_one()[0]
    version = result.split(".")
    return int(version[0])


def is_server_auth_method_upgradable(session) -> bool:
    major_version = get_server_major_version(session)
    return major_version >= 8


def is_password_extensions_supported(session) -> bool:
    # support for dual and random password is from version 8.0
    major_version = get_server_major_version(session)
    return major_version >= 8

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
from mysqlsh.mysql import split_account


def get_current_user(session):
    result = session.run_sql("SELECT current_user()")
    return result.fetch_one()[0]


def collect_account_data(account: str):
    if not account:
        account = get_current_user(globals.shell.get_session())
    account_data = split_account(account)
    account_data["account"] = account
    return account_data, account


def get_credentials():
    return globals.shell.parse_uri(globals.shell.get_session().uri)


def reopen_session_with_password(password: str, error_message: str = None):
    credentials = get_credentials()
    credentials["password"] = password

    try:
        result = globals.shell.open_session(credentials)
        return result, []
    except Error as e:
        return None, [e.msg, error_message]

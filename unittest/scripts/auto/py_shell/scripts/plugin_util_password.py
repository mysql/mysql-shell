#@<> Setup
test_user1 = "passtest"
test_user2 = "othertestpass"
test_host = __host
test_account1 = test_user1 + "@" + test_host
test_account2 = test_user2 + "@" + test_host
test_account_quoted1 = f"\"{test_user1}\"@\"{test_host}\""
test_account_quoted2 = f"\"{test_user2}\"@\"{test_host}\""

default_auth_method = "caching_sha2_password"
if __version_num < 80000:
    default_auth_method = "mysql_native_password"

def recreate_test_user(username: str, password: str = None, auth_method: str = default_auth_method):
    args = [username, test_host]
    session.run_sql("drop user if exists ?@?;", args)
    if password == None:
        session.run_sql("create user ?@?;", args)
    else:
        args.append(auth_method)
        args.append(password)
        session.run_sql("create user ?@? identified with ? by ?;", args)


def get_test_user_uri(username: str, password: str = None):
    conn_data = shell.parse_uri(__mysql_uri)
    conn_data["user"] = username
    conn_data["host"] = test_host
    if password:
        conn_data["password"] = password
    return shell.unparse_uri(conn_data)


#@<> Simple change password
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test")
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "1234")
testutil.expect_password("Confirm new password: ", "1234")
EXPECT_NO_THROWS(lambda: util.change_password())
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "1234")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()


#@<> Change password from other user
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test22")
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "12345")
testutil.expect_password("Confirm new password: ", "12345")
EXPECT_NO_THROWS(lambda: util.change_password({"account": test_account1}))
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "12345")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()


#@<> Change password from other user with insufficient privileges
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test2")
recreate_test_user(test_user2, "fsdff")
shell.connect(get_test_user_uri(test_user2, "fsdff"))
shell.options["useWizards"] = 1
EXPECT_THROWS(lambda: util.change_password({"account": test_account1}), f"Access denied for user '{test_user2}'@'{test_host}'")
shell.options["useWizards"] = 0
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "test2")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)


#@<> Change password to a random one {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "testother")
shell.options["useWizards"] = 1
gen_pass = ""

def rand_password_change():
    global gen_pass
    gen_pass = util.change_password({"account": test_account1, "random": True})

EXPECT_NO_THROWS(lambda: rand_password_change())
del rand_password_change
shell.options["useWizards"] = 0
EXPECT_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "testother")),
              f"Access denied for user '{test_user1}'@'{test_host}'")
EXPECT_NO_THROWS(lambda: shell.connect(
    get_test_user_uri(test_user1, gen_pass)))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)


#@<> Change password and retain old one (dual) {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "1234")
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "dual": True}))
shell.options["useWizards"] = 0
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "1234")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "test")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()


#@<> Warn about dual password {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "1234")
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "dual": True}))

testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "dual": False}))
EXPECT_OUTPUT_CONTAINS(f"WARNING: The account {test_account1} has a retained old (dual) password. "
                       "It can be discarded using util.change_password({\"account\":\"" + test_account1 + "\", \"discardOld\": True}).")

testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
EXPECT_NO_THROWS(lambda: util.change_password({"account": test_account1}))
EXPECT_OUTPUT_CONTAINS(f"WARNING: The account {test_account1} has a retained old (dual) password. "
                       "It can be discarded using util.change_password({\"account\":\"" + test_account1 + "\", \"discardOld\": True}).")
shell.options["useWizards"] = 0
testutil.assert_no_prompts()


#@<> Error when dual a dual password account {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "1234")
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "dual": True}))

EXPECT_THROWS(lambda: util.change_password({"account": test_account1, "dual": True}),
              f"Unable to discard existing dual password implicitly")
EXPECT_OUTPUT_CONTAINS(f"ERROR: The account {test_account1} has a retained old (dual) password. "
                       "Please discard it first using util.change_password({\"account\":\"" + test_account1 + "\", \"discardOld\": True}).")

shell.options["useWizards"] = 0
testutil.assert_no_prompts()


#@<> Discard old password from dual account {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "1234")
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "dual": True}))

testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "discardOld": False}))
EXPECT_OUTPUT_CONTAINS(f"WARNING: The account {test_account1} has a retained old (dual) password. "
                       "It can be discarded using util.change_password({\"account\":\"" + test_account1 + "\", \"discardOld\": True}).")

EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "discardOld": True}))
EXPECT_OUTPUT_CONTAINS("NOTE: Old (dual) password was succesfully discarded.")
shell.options["useWizards"] = 0

EXPECT_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "1234")),
              f"Access denied for user '{test_user1}'@'{test_host}'")

EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "test")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()


#@<> Check if account has dual password {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "1234")
shell.options["useWizards"] = 1

EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "discardOld": True}))
EXPECT_OUTPUT_CONTAINS(
    "NOTE: Account passtest@localhost does not have a dual password present.")
shell.options["useWizards"] = 0
testutil.assert_no_prompts()


#@<> Pass new password as argument
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test")
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
EXPECT_NO_THROWS(lambda: util.change_password({"newPassword": "1234"}))
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "1234")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()


#@<> Check for mysql_native_password {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test", "mysql_native_password")
shell.connect(get_test_user_uri(test_user1, "test"))
EXPECT_OUTPUT_CONTAINS(
    "WARNING: This account uses a deprecated authentication method. Please read \help mysql_native_password help topic for more information.")

WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.change_password(
    {"newPassword": "1234"}))
EXPECT_OUTPUT_CONTAINS(
    f"WARNING: The account {test_account1} is using the deprecated mysql_native_password authentication plugin. "
    "Please use the \"util.change_auth_method\" function to upgrade it to caching_sha2_password.")
testutil.assert_no_prompts()

#@<> Check for mysql_native_password {VER(<8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test", "mysql_native_password")
shell.connect(get_test_user_uri(test_user1, "test"))
EXPECT_OUTPUT_NOT_CONTAINS(
    "WARNING: Authentication method for this account is using the mysql_native_password which was deprecated because of its weak security. "
    "Please use the \"util.change_auth_method\" function to upgrade it to caching_sha2_password.")

WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.change_password(
    {"newPassword": "1234"}))
EXPECT_OUTPUT_NOT_CONTAINS(
    f"WARNING: The account {test_account1} is using the deprecated mysql_native_password authentication plugin. "
    "Please use the \"util.changeAuthMethod\" function to upgrade it to caching_sha2_password.")
testutil.assert_no_prompts()

#@<> Simple authentication plugin update {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test", "mysql_native_password")
# required privilege to update auth
session.run_sql("grant create user on *.* to " + test_account1)
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter password: ", "test")
testutil.expect_password("Confirm password: ", "test")
EXPECT_NO_THROWS(lambda: util.upgrade_auth_method())
shell.options["useWizards"] = 0
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "test")))
WIPE_OUTPUT()
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
session.run_sql(f"SHOW CREATE USER {test_account1}")
EXPECT_OUTPUT_NOT_CONTAINS("mysql_native_password")
EXPECT_OUTPUT_CONTAINS("caching_sha2_password")
shell.connect(__mysql_uri)
testutil.assert_no_prompts()

#@<> Authentication update fail because of server version {VER(<8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test", "mysql_native_password")
# required privilege to update auth
session.run_sql("grant create user on *.* to " + test_account1)
shell.connect(get_test_user_uri(test_user1, "test"))
EXPECT_THROWS(lambda: util.upgrade_auth_method(), "Unsupported server version. To upgrade authentication method to caching_sha2_password, please upgrade the server to version at least 8.0.")
shell.connect(__mysql_uri)
testutil.assert_no_prompts()

#@<> Account already has been updated to caching_sha2_password {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test")
# required privilege to update auth
session.run_sql("grant create user on *.* to " + test_account1)
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
EXPECT_NO_THROWS(lambda: util.upgrade_auth_method())
EXPECT_OUTPUT_CONTAINS(
    f"NOTE: Account {test_account1} is not using \"mysql_native_password\". No changes were made.")
shell.options["useWizards"] = 0
shell.connect(__mysql_uri)
testutil.assert_no_prompts()

#@<> Failed authentication plugin update due to privileges {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test", "mysql_native_password")
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter password: ", "test")
testutil.expect_password("Confirm password: ", "test")
EXPECT_THROWS(lambda: util.upgrade_auth_method(), "Failed to change authentication method: Access denied; you need (at least one of) the CREATE USER privilege(s) for this operation")
shell.options["useWizards"] = 0
testutil.assert_no_prompts()

#@<> Authentication plugin update of another account {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test", "mysql_native_password")
shell.options["useWizards"] = 1
testutil.expect_password("Enter password: ", "1234")
testutil.expect_password("Confirm password: ", "1234")
EXPECT_NO_THROWS(lambda: util.upgrade_auth_method({"account": test_account1}))
shell.options["useWizards"] = 0
WIPE_OUTPUT()
session.run_sql(f"SHOW CREATE USER {test_account1}")
EXPECT_OUTPUT_NOT_CONTAINS("mysql_native_password")
EXPECT_OUTPUT_CONTAINS("caching_sha2_password")
testutil.assert_no_prompts()

#@<> Failed authentication plugin update of another account {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test", "mysql_native_password")
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter password: ", "1234")
testutil.expect_password("Confirm password: ", "1234")
EXPECT_THROWS(lambda: util.upgrade_auth_method({"account": test_account1}), "Failed to change authentication method: Access denied; you need (at least one of) the CREATE USER privilege(s) for this operation")
shell.options["useWizards"] = 0
testutil.assert_no_prompts()

#@<> Changing auth plugin with password arg {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test", "mysql_native_password")
shell.options["useWizards"] = 1
EXPECT_NO_THROWS(lambda: util.upgrade_auth_method({"password": "1234", "account": test_account1}))
shell.options["useWizards"] = 0
WIPE_OUTPUT()
session.run_sql(f"SHOW CREATE USER {test_account1}")
EXPECT_OUTPUT_NOT_CONTAINS("mysql_native_password")
EXPECT_OUTPUT_CONTAINS("caching_sha2_password")
testutil.assert_no_prompts()

#@<> Entering mistmatched password above 3 times {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test")
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "tasdas")
testutil.expect_password("Confirm new password: ", "wrong")
testutil.expect_password("Enter new password: ", "asdda")
testutil.expect_password("Confirm new password: ", "wrong")
testutil.expect_password("Enter new password: ", "setrss")
testutil.expect_password("Confirm new password: ", "wrong")
EXPECT_THROWS(lambda: util.change_password(),
              "Failed to enter matching passwords.")
shell.options["useWizards"] = 0
testutil.assert_no_prompts()


#@<> Entering new password max 3 times
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test")
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "1234")
testutil.expect_password("Confirm new password: ", "asdas")
testutil.expect_password("Enter new password: ", "1234")
testutil.expect_password("Confirm new password: ", "234")
testutil.expect_password("Enter new password: ", "1234")
testutil.expect_password("Confirm new password: ", "1234")
EXPECT_NO_THROWS(lambda: util.change_password())
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "1234")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()

#@<> Entering new password by second time
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "test")
shell.connect(get_test_user_uri(test_user1, "test"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "1234")
testutil.expect_password("Confirm new password: ", "asdas")
testutil.expect_password("Enter new password: ", "1234")
testutil.expect_password("Confirm new password: ", "1234")
EXPECT_NO_THROWS(lambda: util.change_password())
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "1234")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()

#@<> Error when auth method upgrading a dual password account {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "1234", "mysql_native_password")
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
EXPECT_NO_THROWS(lambda: util.change_password(
    {"account": test_account1, "dual": True}))

EXPECT_THROWS(lambda: util.upgrade_auth_method({"password": "test", "account": test_account1}),
              f"Unable to discard existing dual password implicitly")
EXPECT_OUTPUT_CONTAINS(f"ERROR: The account {test_account1} has a retained old (dual) password. It will be lost on auth method replacement. "
                       "Please discard it first using util.change_password({\"account\":\"" + test_account1 + "\", \"discardOld\": True}).")

shell.options["useWizards"] = 0
testutil.assert_no_prompts()

#@<> Setup password policy validation {VER(>=8.0.0)}
shell.connect(__mysql_uri)
ensure_plugin_enabled('validate_password', session)
session.run_sql("set global validate_password_policy=MEDIUM")

#@<> Entering password for policy validation above 3 times {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "Testpass1234?")
shell.connect(get_test_user_uri(test_user1, "Testpass1234?"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
testutil.expect_password("Enter new password: ", "othertest")
testutil.expect_password("Confirm new password: ", "othertest")
testutil.expect_password("Enter new password: ", "Nexttest")
testutil.expect_password("Confirm new password: ", "Nexttest")
EXPECT_THROWS(lambda: util.change_password(),
              "Failed to change password: Your password does not satisfy the current policy requirements")
shell.options["useWizards"] = 0
testutil.assert_no_prompts()

#@<> Entering password for policy validation max 3 times {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "Testpass1234?")
shell.connect(get_test_user_uri(test_user1, "Testpass1234?"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "test")
testutil.expect_password("Confirm new password: ", "test")
testutil.expect_password("Enter new password: ", "othertest")
testutil.expect_password("Confirm new password: ", "othertest")
testutil.expect_password("Enter new password: ", "Asdfg45s?")
testutil.expect_password("Confirm new password: ", "Asdfg45s?")
EXPECT_NO_THROWS(lambda: util.change_password())
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "Asdfg45s?")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()

#@<> Entering password for policy validation max 2 times {VER(>=8.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "Testpass1234?")
shell.connect(get_test_user_uri(test_user1, "Testpass1234?"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter new password: ", "othertest")
testutil.expect_password("Confirm new password: ", "othertest")
testutil.expect_password("Enter new password: ", "Asdfg45s?")
testutil.expect_password("Confirm new password: ", "Asdfg45s?")
EXPECT_NO_THROWS(lambda: util.change_password())
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "Asdfg45s?")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()

#@<> Entering password for mysql_native_password upgrade for policy validation above 3 times {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "Testpass1234?", "mysql_native_password")
session.run_sql("grant create user on *.* to " + test_account1)
shell.connect(get_test_user_uri(test_user1, "Testpass1234?"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter password: ", "test")
testutil.expect_password("Confirm password: ", "test")
testutil.expect_password("Enter password: ", "othertest")
testutil.expect_password("Confirm password: ", "othertest")
testutil.expect_password("Enter password: ", "Nexttest")
testutil.expect_password("Confirm password: ", "Nexttest")
EXPECT_THROWS(lambda: util.upgrade_auth_method(),
              "Failed to change authentication method: Your password does not satisfy the current policy requirements")
shell.options["useWizards"] = 0
testutil.assert_no_prompts()

#@<> Entering password for mysql_native_password upgrade for policy validation max 3 times {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "Testpass1234?", "mysql_native_password")
session.run_sql("grant create user on *.* to " + test_account1)
shell.connect(get_test_user_uri(test_user1, "Testpass1234?"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter password: ", "test")
testutil.expect_password("Confirm password: ", "test")
testutil.expect_password("Enter password: ", "othertest")
testutil.expect_password("Confirm password: ", "othertest")
testutil.expect_password("Enter password: ", "Asdfg45s?")
testutil.expect_password("Confirm password: ", "Asdfg45s?")
EXPECT_NO_THROWS(lambda: util.upgrade_auth_method())
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Authentication and password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "Asdfg45s?")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()

#@<> Entering password for mysql_native_password upgrade for policy validation max 2 times {VER(>=8.0.0) and VER(<9.0.0)}
shell.connect(__mysql_uri)
recreate_test_user(test_user1, "Testpass1234?", "mysql_native_password")
session.run_sql("grant create user on *.* to " + test_account1)
shell.connect(get_test_user_uri(test_user1, "Testpass1234?"))
shell.options["useWizards"] = 1
testutil.expect_password("Enter password: ", "othertest")
testutil.expect_password("Confirm password: ", "othertest")
testutil.expect_password("Enter password: ", "Asdfg45s?")
testutil.expect_password("Confirm password: ", "Asdfg45s?")
EXPECT_NO_THROWS(lambda: util.upgrade_auth_method())
shell.options["useWizards"] = 0
EXPECT_OUTPUT_CONTAINS("NOTE: Authentication and password has been successfully updated.")
EXPECT_NO_THROWS(lambda: shell.connect(get_test_user_uri(test_user1, "Asdfg45s?")))
session.run_sql("select current_user();")
EXPECT_OUTPUT_CONTAINS(test_account1)
testutil.assert_no_prompts()

#@<> Cleanup
shell.connect(__mysql_uri)
ensure_plugin_disabled('validate_password', session)
session.run_sql(f"drop user if exists {test_account1};")
session.run_sql(f"drop user if exists {test_account2};")
#@<> {VER(>8.0.0)}
#@<> Create sample user and store the password in the credential helper
current_helper = shell.options["credentialStore.helper"]
shell.options["credentialStore.helper"] = "plaintext"
shell.options.useWizards = True
shell.delete_all_credentials()


# Create the sample user
shell.connect(__mysqluripwd)
session.run_sql("DROP USER IF EXISTS sample@localhost")
session.run_sql("CREATE USER sample@localhost IDENTIFIED WITH caching_sha2_password by 'password'")
session.run_sql("GRANT ALL on *.* to sample@localhost")
shell.disconnect()

# Connect with the sample user and store the credentials
sample_user = __mysql_uri.replace('root', 'sample')
testutil.expect_password(f"Please provide the password for '{sample_user}':", "password")
testutil.expect_prompt(f"Save password for '{sample_user}'?", "Y")
shell.connect(sample_user)
shell.disconnect()

parameters = [
    sample_user,
    "--py",
    "--log-level=debug2",
    "--credential-store-helper=plaintext",
    "--passwords-from-stdin",
     "-e", "print(session)",
]
#@<> Success: no password provided, logged in OK
WIPE_SHELL_LOG()
testutil.call_mysqlsh(parameters, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"], "mysqlshrec")
EXPECT_SHELL_LOG_CONTAINS_COUNT(f"Retrieving password from credential manager", 1)
EXPECT_STDOUT_CONTAINS(f"<ClassicSession:{sample_user}>")

#@<> Forcing password prompt with -p, used wrong password
WIPE_SHELL_LOG()
testutil.call_mysqlsh(parameters + ["-p"], "wrong_password", ["MYSQLSH_TERM_COLOR_MODE=nocolor"], "mysqlshrec")
EXPECT_SHELL_LOG_NOT_CONTAINS("Retrieving password from credential manager")
EXPECT_STDOUT_CONTAINS("MySQL Error 1045 (28000): Access denied for user 'sample'@'localhost' (using password: YES)")

#@<> Forcing password prompt with -p, used correct password
WIPE_SHELL_LOG()
testutil.call_mysqlsh(parameters + ["-p"], "password", ["MYSQLSH_TERM_COLOR_MODE=nocolor"], "mysqlshrec")
EXPECT_SHELL_LOG_NOT_CONTAINS("Retrieving password from credential manager")
EXPECT_STDOUT_CONTAINS(f"<ClassicSession:{sample_user}>")


#@<> Again, using the stored password
testutil.call_mysqlsh(parameters, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"], "mysqlshrec")
EXPECT_STDOUT_CONTAINS(f"<ClassicSession:{sample_user}>")

#@<> Dump, using the stored password just once for the initial connection, not for the dump operation
shell.connect(__mysqluripwd)
session.run_sql('drop schema if exists connection_handling')
session.run_sql('create schema connection_handling')
session.run_sql('create table connection_handling.sample(id int)')
session.close()
WIPE_SHELL_LOG()
testutil.call_mysqlsh(parameters[:5] + ["--", "util", "dump-instance", "testing"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"], "mysqlshrec")
EXPECT_SHELL_LOG_CONTAINS_COUNT("Retrieving password from credential manager", 1)

#@<> Load, using the stored password just once for the initial connection, not for the load operation
shell.connect(__mysqluripwd)
session.run_sql('drop schema connection_handling')
session.close()
WIPE_SHELL_LOG()
testutil.call_mysqlsh(parameters[:5] + ["--", "util", "load-dump", "testing"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"], "mysqlshrec")
EXPECT_SHELL_LOG_CONTAINS_COUNT("Retrieving password from credential manager", 1)

#@<> Cleanup
testutil.rmdir('testing', True)
shell.options["credentialStore.helper"] = current_helper
shell.connect(__mysqluripwd)
session.run_sql("DROP USER IF EXISTS sample@localhost")
session.run_sql('drop schema if exists connection_handling')
shell.disconnect()

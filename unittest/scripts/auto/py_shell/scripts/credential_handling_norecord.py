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


prompts = f"Please provide the password for 'sample@localhost': |"
parameters = [
    sample_user,
    "--py",
    "--credential-store-helper=plaintext",
    "--passwords-from-stdin",
     "-e", "print(session)"
]
#@<> Success: no password provided, logged in OK
testutil.call_mysqlsh(parameters, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS(f"<ClassicSession:{sample_user}>")

#@<> Forcing password prompt with -p, used wrong password
testutil.call_mysqlsh(parameters + ["-p"], "wrong_password", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("MySQL Error 1045 (28000): Access denied for user 'sample'@'localhost' (using password: YES)")

#@<> Forcing password prompt with -p, used correct password
testutil.call_mysqlsh(parameters + ["-p"], "password", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS(f"<ClassicSession:{sample_user}>")

#@<> Again, using the stored password
testutil.call_mysqlsh(parameters, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS(f"<ClassicSession:{sample_user}>")

#@<> Cleanup
shell.options["credentialStore.helper"] = current_helper
shell.connect(__mysqluripwd)
session.run_sql("DROP USER IF EXISTS sample@localhost")
shell.disconnect()

// Validate that a valid value can be set to the Shell option "credentialStore.helper"

//@ Set a valid value for the shell.options["credentialStore.helper"] depending on the OS that is in use.
shell.options["credentialStore.helper"] = "plaintext";

//@ Verify that the shell.options["credentialStore.helper"] option has the value you set
shell.options["credentialStore.helper"];

//@ [SR6: always]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it (set to always).
shell.options["credentialStore.savePasswords"] = "always";

//@ Create a Session to a server using valid credentials without the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.mysql.uri + "':", __cred.pwd);
shell.connect(__cred.mysql.uri);

//@ [SR12]Verify that the credentials are stored using the function shell.listCredentials()
EXPECT_ARRAY_CONTAINS(__cred.mysql.uri, shell.listCredentials());

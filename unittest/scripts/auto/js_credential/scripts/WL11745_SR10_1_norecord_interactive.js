// Verify that the function shell.deleteCredential(url) delete credentials previously saved

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@ [SR6]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it.
shell.options["credentialStore.savePasswords"] = "always";

//@ Create a Session to a server using valid credentials including the password
// The Session must be created successfully
shell.connect(__cred.x.uri_pwd);

//@ Call the function shell.deleteCredential(url) using the same credentials
// The credentials must be deleted from the storage
shell.deleteCredential(__cred.x.uri);

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
EXPECT_ARRAY_NOT_CONTAINS(__cred.x.uri, shell.listCredentials());

//@ Create a new Session to a server using the same credentials but not including the password
// A prompt to type the password must be displayed, type the password
testutil.expectPassword("Please provide the password for '" + __cred.x.uri + "':", __cred.pwd);
// The Session must be created successfully
shell.connect(__cred.x.uri);

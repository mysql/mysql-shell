//@ Disable the credential store
shell.options["credentialStore.helper"] = "<disabled>";

//@ Validate the helper value
shell.options["credentialStore.helper"];

//@ Set the shell.options["credentialStore.savePasswords"] to 'prompt'.
shell.options["credentialStore.savePasswords"] = "prompt";

//@ Create a Session to a server using valid credentials but not passing the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.x.uri + "':", __cred.pwd);
// No prompt to save the password
shell.connect(__cred.x.uri);

//@ Create a Session to a server using valid credentials with correct password
// No prompt to save the password
shell.connect(__cred.x.uri_pwd);

//@ The listCredentialHelpers() function should work just fine
EXPECT_ARRAY_CONTAINS("plaintext", shell.listCredentialHelpers());

//@ The storeCredential() function should throw
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.x.uri + "':", __cred.pwd);
shell.storeCredential(__cred.x.uri);

//@ The storeCredential() with password function should throw
shell.storeCredential(__cred.x.uri, __cred.pwd);

//@ The deleteCredential() function should throw
shell.deleteCredential(__cred.x.uri);

//@ The deleteAllCredentials() function should throw
shell.deleteAllCredentials();

//@ The listCredentials() function should throw
shell.listCredentials();

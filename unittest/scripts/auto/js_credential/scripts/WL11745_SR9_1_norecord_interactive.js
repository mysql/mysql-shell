// Verify that the function shell.storeCredential(url, password) save credentials for future session creation

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@ [SR6]Set the shell.options["credentialStore.savePasswords"] to 'never'.
shell.options["credentialStore.savePasswords"] = "never";

//@ Call the function shell.storeCredential(url, password) giving valid credentials
shell.storeCredential(__cred.x.uri, __cred.pwd);

//@ [SR12]Verify that the credentials are stored using the function shell.listCredentials()
EXPECT_ARRAY_CONTAINS(__cred.x.uri, shell.listCredentials());

//@ Create a Session to a server using the same credentials without passing the password
// The Session must be created successfully
shell.connect(__cred.x.uri);

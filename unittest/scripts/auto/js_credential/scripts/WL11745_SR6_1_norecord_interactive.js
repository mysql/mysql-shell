// Verify that when the shell.options["credentialStore.savePasswords"] option is set to 'never', the credentials given to create Sessions are not stored

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@ Set the shell.options["credentialStore.savePasswords"] to 'never'
shell.options["credentialStore.savePasswords"] = "never";

//@ Create a Session to a server using valid credentials including the password
// The Session must be created successfully
shell.connect(__cred.x.uri_pwd);

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
EXPECT_ARRAY_NOT_CONTAINS(__cred.x.uri, shell.listCredentials());

//@ Create another Session to a server using different credentials including the password
// The Session must be created successfully
shell.connect(__cred.second.uri_pwd);

//@ [SR12]Verify that different credentials are not stored using the function shell.listCredentials()
EXPECT_ARRAY_NOT_CONTAINS(__cred.second.uri, shell.listCredentials());

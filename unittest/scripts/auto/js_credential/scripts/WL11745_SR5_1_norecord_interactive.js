// Verify that passing complete credentials (user and password) are not saved automatically and cannot be used in future session creation

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@ [SR6]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it.
shell.options["credentialStore.savePasswords"] = "always";

//@ Create a Session to a server using valid credentials including the password
// The Session must be created successfully
shell.connect(__cred.x.uri_pwd);

//@ Create a new Session to a server using the previous credentials given but not passing the password
// Type the password when prompted
testutil.expectPassword("Please provide the password for '" + __cred.x.uri + "':", __cred.pwd);
shell.connect(__cred.x.uri);

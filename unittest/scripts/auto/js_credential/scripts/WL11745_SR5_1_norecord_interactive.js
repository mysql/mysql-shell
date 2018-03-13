// Verify that passing complete credentials (user and password) are saved automatically and are used in future session creation

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";

//@ [SR6]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it.
shell.options["credentialStore.savePasswords"] = "always";

//@ Create a Session to a server using valid credentials including the password
// The Session must be created successfully
shell.connect(__cred.x.uri_pwd);

//@ Create a new Session to a server using the previous credentials given but not passing the password
// The Session must be created successfully
shell.connect(__cred.x.uri);

//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";
shell.options["credentialStore.savePasswords"] = "prompt";

//@ Create a Session to a server using valid credentials but not passing the password
// No prompt for password
// No prompt to save the password
// Should try to connect without password and fail
shell.connect(__cred.x.uri);

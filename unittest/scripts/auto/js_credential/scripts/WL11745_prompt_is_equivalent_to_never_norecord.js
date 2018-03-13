//@ Initialization
shell.options["credentialStore.helper"] = "plaintext";
shell.options["credentialStore.savePasswords"] = "prompt";

//@ Create a Session to a server using valid credentials
// No prompt to save the password
shell.connect(__cred.x.uri_pwd);

//@ Store valid credentials
shell.storeCredential(__cred.x.uri, __cred.pwd);

//@ Create a Session to a server using valid credentials but not passing the password
shell.connect(__cred.x.uri);

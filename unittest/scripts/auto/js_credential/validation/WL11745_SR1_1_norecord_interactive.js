// Validate that the Shell option "credentialStore.helper" already has a value set depending on the OS that is in use

//@ Verify that the shell.options["credentialStore.helper"] option is set to a valid value for the OS that is in use.
|default|

//@ [SR6: always]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it (set to always).
|always|

//@ Create a Session to a server using valid credentials including the password
|<Session:<<<__cred.x.uri>>>>|

//@ [SR12]Verify that the credentials are stored using the function shell.listCredentials()
||

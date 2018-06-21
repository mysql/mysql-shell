// Validate that a valid value can be set to the Shell option "credentialStore.helper"

//@ Set a valid value for the shell.options["credentialStore.helper"] depending on the OS that is in use.
|plaintext|

//@ Verify that the shell.options["credentialStore.helper"] option has the value you set
|plaintext|

//@ [SR6: always]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it (set to always).
|always|

//@ Create a Session to a server using valid credentials without the password
|<ClassicSession:<<<__cred.mysql.uri>>>>|

//@ [SR12]Verify that the credentials are stored using the function shell.listCredentials()
||

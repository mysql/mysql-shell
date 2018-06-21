// Verify that the function shell.deleteAllCredentials() delete all the credentials previously saved

//@ Initialization
|plaintext|

//@ [SR6]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it.
|always|

//@ Create a Session to a server using valid credentials without the password
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

//@ Create another Session to a server using different and valid credentials without the password
// The Session must be created successfully
|<Session:<<<__cred.second.uri>>>>|

//@ Create another Session to a server using third credentials without the password
// The Session must be created successfully
|<Session:<<<__cred.third.uri>>>>|

//@ [SR12]Call the function shell.listCredentials()
||

//@ All the credentials previously saved must be displayed
||

//@ Call the function shell.deleteAllCredentials(), all the credentials previously saved must be deleted
||

//@ [SR12]Call the function shell.listCredentials() to confirm
||

//@ No credentials must be displayed
||

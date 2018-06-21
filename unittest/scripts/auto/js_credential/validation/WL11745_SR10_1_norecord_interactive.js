// Verify that the function shell.deleteCredential(url) delete credentials previously saved

//@ Initialization
|plaintext|

//@ [SR6]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it.
|always|

//@ Create a Session to a server using valid credentials without the password
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

//@ Call the function shell.deleteCredential(url) using the same credentials
// The credentials must be deleted from the storage
||

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
||

//@ Create a new Session to a server using the same credentials but not including the password
// A prompt to type the password must be displayed, type the password
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

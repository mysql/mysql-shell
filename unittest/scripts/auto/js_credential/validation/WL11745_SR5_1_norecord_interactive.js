// Verify that passing complete credentials (user and password) are saved automatically and are used in future session creation

//@ Initialization
|plaintext|

//@ [SR6]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it.
|always|

//@ Create a Session to a server using valid credentials including the password
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

//@ Create a new Session to a server using the previous credentials given but not passing the password
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

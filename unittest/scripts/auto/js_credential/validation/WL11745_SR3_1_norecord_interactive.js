// Verify that the credentials are saved and are used in future session creation

//@ Initialization
|plaintext|

//@ [SR6: always]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it (set to always).
|always|

//@ Create a Session to a server using valid credentials but not passing the password
// Type the password when prompted
|<Session:<<<__cred.x.uri>>>>|

//@ Create a new Session to a server using the previous credentials given and not passing the password
// No prompt for password must be displayed and the Session must be created successfully.
|<Session:<<<__cred.x.uri>>>>|

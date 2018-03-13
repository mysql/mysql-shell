// Verify that when the shell.options["credentialStore.savePasswords"] option is set to 'never', the credentials given to create Sessions are not stored

//@ Initialization
|plaintext|

//@ Set the shell.options["credentialStore.savePasswords"] to 'never'
|never|

//@ Create a Session to a server using valid credentials including the password
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
||

//@ Create another Session to a server using different credentials including the password
// The Session must be created successfully
|<Session:<<<__cred.second.uri>>>>|

//@ [SR12]Verify that different credentials are not stored using the function shell.listCredentials()
||

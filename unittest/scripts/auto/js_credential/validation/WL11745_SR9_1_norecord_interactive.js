// Verify that the function shell.storeCredential(url, password) save credentials for future session creation

//@ Initialization
|plaintext|

//@ [SR6]Set the shell.options["credentialStore.savePasswords"] to 'never'.
|never|

//@ Call the function shell.storeCredential(url, password) giving valid credentials
||

//@ [SR12]Verify that the credentials are stored using the function shell.listCredentials()
||

//@ Create a Session to a server using the same credentials without passing the password
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

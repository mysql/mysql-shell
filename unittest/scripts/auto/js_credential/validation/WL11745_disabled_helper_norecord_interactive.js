//@ Disable the credential store
|<disabled>|

//@ Validate the helper value
|<disabled>|

//@ Set the shell.options["credentialStore.savePasswords"] to 'prompt'.
|prompt|

//@ Create a Session to a server using valid credentials but not passing the password
// Type the password when prompted
// No prompt to save the password
|<Session:<<<__cred.x.uri>>>>|

//@ Create a Session to a server using valid credentials with correct password
// No prompt to save the password
|<Session:<<<__cred.x.uri>>>>|

//@ The listCredentialHelpers() function should work just fine
||

//@ The storeCredential() function should throw
||Cannot save the credential, current credential helper is invalid (RuntimeError)

//@ The storeCredential() with password function should throw
||Cannot save the credential, current credential helper is invalid (RuntimeError)

//@ The deleteCredential() function should throw
||Cannot delete the credential, current credential helper is invalid (RuntimeError)

//@ The deleteAllCredentials() function should throw
||Cannot delete all credentials, current credential helper is invalid (RuntimeError)

//@ The listCredentials() function should throw
||Cannot list credentials, current credential helper is invalid (RuntimeError)

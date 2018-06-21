// Verify that when the shell.options["credentialStore.savePasswords"] option is set to 'prompt', a prompt asks the user if he want to save the credentials

//@ Initialization
|plaintext|

//@ Set the shell.options["credentialStore.savePasswords"] to 'prompt'
|prompt|

//@ Create a Session to a server using valid credentials without the password
// Verify that a prompt asking if you want to save the credentials is displayed, answer 'yes'
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

//@ [SR12]Verify that the credentials are stored using the function shell.listCredentials()
||

//@ Create another Session to a server using different credentials without the password
// Verify that a prompt asking if you want to save the credentials is displayed, answer 'no'
// The Session must be created successfully
|<Session:<<<__cred.second.uri>>>>|

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
||

//@ Create another Session to a server using third credentials without the password
// [SR7]Verify that a prompt asking if you want to save the credentials is displayed, answer 'never'
// The Session must be created successfully
|<Session:<<<__cred.third.uri>>>>|

//@ [SR12]Verify that the third credentials are not stored using the function shell.listCredentials()
||

//@ Verify that the URI used to create the session is listed in the shell.options["credentialStore.excludeFilters"] option
||

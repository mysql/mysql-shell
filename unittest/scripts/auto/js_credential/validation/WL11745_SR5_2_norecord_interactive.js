// Verify that passing complete credentials (user and password) are not saved automatically if the URI is on the 'credentialStore.excludeFilters' option

//@ Initialization
|plaintext|

//@ [SR6]Verify that shell.options["credentialStore.savePasswords"] is enabled, if not then enable it (set to 'always').
|always|

//@<OUT> Add the URI that you're going to use to create a session to the shell.options["credentialStore.excludeFilters"] option
[
    "<<<__cred.x.uri>>>"
]


//@ Create a Session to a server using valid credentials including the password
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
||

//@ Create a new Session to a server using the previous credentials given but not passing the password
// The Session creation must prompt the user for the password
|<Session:<<<__cred.x.uri>>>>|

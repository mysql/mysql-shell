// Verify that invalid credentials removes an old entry from the credentials saved

//@ Initialization
|plaintext|

//@ [SR6]Set the shell.options["credentialStore.savePasswords"] to 'prompt'.
|prompt|

//@ Create a Session to a server using valid credentials but not passing the password
// Type the password when prompted
// Answer 'yes' to the prompt that asks if you want to save the credentials
|<Session:<<<__cred.x.uri>>>>|

//@ Update the password for the user used in the session created
|Query OK|

//@ Create a Session to a server using the previous credentials given and not passing the password
// A prompt to type the password must be displayed, type the new password
// Answer 'no' to the prompt that asks if you want to save the credentials
// The Session must be created successfully
|<Session:<<<__cred.x.uri>>>>|

//@ A message about the invalid credentials must be printed {VER(>=8.0.12)}
||

//@ A message about the invalid credentials must be printed - old message {VER(<8.0.12)}
||

//@ [SR12]Verify that the credentials are not stored using the function shell.listCredentials()
||

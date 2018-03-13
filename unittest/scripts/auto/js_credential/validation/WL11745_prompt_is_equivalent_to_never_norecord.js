//@ Initialization
|plaintext|
|prompt|

//@ Create a Session to a server using valid credentials
// No prompt to save the password
|<Session:<<<__cred.x.uri>>>>|

//@ Store valid credentials
||

//@ Create a Session to a server using valid credentials but not passing the password
|<Session:<<<__cred.x.uri>>>>|

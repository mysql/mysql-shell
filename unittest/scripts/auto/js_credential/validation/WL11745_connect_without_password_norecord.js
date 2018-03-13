//@ Initialization
|plaintext|
|prompt|

//@ Create a Session to a server using valid credentials but not passing the password {VER(>=8.0.12)}
// No prompt for password
// No prompt to save the password
// Should try to connect without password and fail
||Access denied for user '<<<__cred.user>>>'@'<<<__cred.host>>>' (using password: NO) (MySQL Error 1045)

//@ Create a Session to a server using valid credentials but not passing the password {VER(<8.0.12)}
||Invalid user or password (MySQL Error 1045)

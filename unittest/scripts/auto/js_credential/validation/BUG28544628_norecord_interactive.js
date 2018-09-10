//@ use plaintext helper
|plaintext|

//@ prompt to save passwords
|prompt|

//@ Create a Class Session to a server using valid credentials without the password
|<ClassicSession:<<<__cred.user>>>@<<<hostname>>>:<<<__mysql_port>>>>|

//@ Create another Classic Session using the same credentials, stored password should be used
|<ClassicSession:<<<__cred.user>>>@<<<hostname>>>:<<<__mysql_port>>>>|

//@<OUT> Check if stored credential matches the expected one
[
    "<<<credential>>>"
]

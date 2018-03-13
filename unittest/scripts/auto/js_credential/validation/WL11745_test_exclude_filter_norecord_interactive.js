//@ Initialization
|plaintext|
|prompt|

//@ Should not prompt to save the password - exclude everything
|<Session:<<<__cred.x.uri>>>>|

//@ Should not prompt to save the password - exact match
|<Session:<<<__cred.x.uri>>>>|

//@ Should not prompt to save the password - wildcards - username
|<Session:<<<__cred.x.uri>>>>|

//@ Should not prompt to save the password - wildcards - host
|<Session:<<<__cred.x.uri>>>>|

//@ Should not prompt to save the password - wildcards - port
|<Session:<<<__cred.x.uri>>>>|

//@ Should not prompt to save the password - wildcards - multiple characters
|<Session:<<<__cred.x.uri>>>>|

//@ Should not prompt to save the password - wildcards - ?
|<Session:<<<__cred.x.uri>>>>|

//@ Should not prompt to save the password - wildcards - ? and *
|<Session:<<<__cred.x.uri>>>>|

//@ Should not prompt to save the password - wildcards - host partial match
|<Session:<<<__cred.x.uri>>>>|

//@ GlobalSetUp
|Query OK|

//@ password on command line
|WARNING: Using a password on the command line interface can be insecure.|
|<ClassicSession:|

//@ no password
|<ClassicSession:|

//@ no password and password prompt
|Conflicting options: --password and --no-password option cannot be used together.|

//@ no password and empty password
|<ClassicSession:|

//@ no password and password
|Conflicting options: --no-password cannot be used if password is provided.|

//@ no password and URI empty password
|<ClassicSession:|

//@ no password and URI password
|Conflicting options: --no-password cannot be used if password is provided in URI.|

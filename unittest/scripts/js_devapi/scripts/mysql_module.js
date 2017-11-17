// Assumptions: validateMember exists

var mysql = require('mysql');

// The tests assume the next variables have been put in place
// on the JS Context
// __uri: <user>@<host>
// __host: <host>
// __port: <port>
// __user: <user>
// __uripwd: <uri>:<pwd>@<host>
// __pwd: <pwd>

//@ mysql module: exports
var exports = dir(mysql);
print('Exported Items:', exports.length);
validateMember(exports, 'getClassicSession');
validateMember(exports, 'getSession');
validateMember(exports, 'help');

//@<OUT> help
mysql.help()

//@<OUT> Help on getClassicSession
mysql.help('getClassicSession');

//@# getClassicSession errors
mysql.getClassicSession()
mysql.getClassicSession(1, 2, 3)
mysql.getClassicSession(["bla"])
mysql.getClassicSession("some@uri", 25)


//@<OUT> Help on getSession
mysql.help('getSession');

//@# getSession errors
mysql.getSession()
mysql.getSession(1, 2, 3)
mysql.getSession(["bla"])
mysql.getSession("some@uri", 25)

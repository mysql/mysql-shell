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

print('getClassicSession:', typeof mysql.getClassicSession);
print('help:', typeof mysql.help);

//@<OUT> Help on getClassicSession
mysql.help('getClassicSession');

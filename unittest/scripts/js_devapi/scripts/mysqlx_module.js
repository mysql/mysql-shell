var mysqlx = require('mysqlx');

// The tests assume the next variables have been put in place
// on the JS Context
// __uri: <user>@<host>
// __host: <host>
// __port: <port>
// __user: <user>
// __uripwd: <uri>:<pwd>@<host>
// __pwd: <pwd>


//@ mysqlx module: exports
var exports = dir(mysqlx);
print('Exported Items:', exports.length);

print('getSession:', typeof mysqlx.getSession, '\n');
print('expr:', typeof mysqlx.expr, '\n');
print('dateValue:', typeof mysqlx.dateValue, '\n');
print('help:', typeof mysqlx.dateValue, '\n');
print('Type:', mysqlx.Type, '\n');
print('IndexType:', mysqlx.IndexType, '\n');

//@# mysqlx module: expression errors
var expr;
expr = mysqlx.expr();
expr = mysqlx.expr(5);

//@ mysqlx module: expression
expr = mysqlx.expr('5+6');
print(expr);

//@<OUT> Help on getSession
mysqlx.help('getSession');

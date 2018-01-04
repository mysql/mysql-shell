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

//@ mysqlx module: dateValue() diffrent parameters
mysqlx.dateValue(2025, 10, 15);
mysqlx.dateValue(2017, 12, 10, 10, 10, 10);
mysqlx.dateValue(2017, 12, 10, 10, 10, 10, 500000);
mysqlx.dateValue(2017, 12, 10, 10, 10, 10, 599999);

//@ mysqlx module: Bug #26429377
mysqlx.dateValue();

//@ mysqlx module: Bug #26429377 - 4/5 arguments
mysqlx.dateValue(1410, 7, 15, 10);

//@ mysqlx module: Bug #26429426
mysqlx.dateValue(1, 2, -19);

//@ month validation
mysqlx.dateValue(1410, 0, 10);

//@ year validation
mysqlx.dateValue(-10, 11, 10);

//@ hour validation
mysqlx.dateValue(1910, 11, 10, 24, 59, 59);

//@ minute validation
mysqlx.dateValue(1910, 11, 10, 23, 60, 59);

//@ second validation
mysqlx.dateValue(1910, 11, 10, 23, 59, -1);

//@ usecond validation
mysqlx.dateValue(1910, 11, 10, 23, 10, 1, 1000000);

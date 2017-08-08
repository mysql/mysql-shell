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


//@ mysqlx module: getSession through URI
mySession = mysqlx.getSession(__uripwd);

print(mySession, '\n');

if (mySession.uri == __displayuri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');

mySession.close();

//@ mysqlx module: getSession through URI and password
mySession = mysqlx.getSession(__uri, __pwd);

print(mySession, '\n');

if (mySession.uri == __displayuri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');

mySession.close();


//@ mysqlx module: getSession through data
var data = { host: __host,
						 port: __port,
						 schema: __schema,
						 dbUser: __user,
						 dbPassword: __pwd };


mySession = mysqlx.getSession(data);

print(mySession, '\n');

if (mySession.uri == __displayuridb)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');

mySession.close();

//@ mysqlx module: getSession through data and password
var data = { host: __host,
						 port: __port,
						 schema: __schema,
						 dbUser: __user};


mySession = mysqlx.getSession(data, __pwd);

print(mySession, '\n');

if (mySession.uri == __displayuridb)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');

//@ mysqlx module: getSession using SSL in URI
var res = mySession.sql('select @@have_ssl').execute();
var row = res.fetchOne();
var have_ssl = (row[0] == 'YES');

var uri_value = 'DISABLED';
if (have_ssl)
  ssl_mode = 'REQUIRED';

var mySslSession = mysqlx.getSession(mySession.uri + "?ssl-mode=" + ssl_mode);

if (mySslSession.uri == (__displayuridb + "?ssl-mode=" + ssl_mode))
  print('Session using right SSL URI\n');
else
  print('Session using wrong SSL URI\n');


mySslSession.close();

mySession.close();

//@# mysqlx module: expression errors
var expr;
expr = mysqlx.expr();
expr = mysqlx.expr(5);

//@ mysqlx module: expression
expr = mysqlx.expr('5+6');
print(expr);

//@<OUT> Help on getSession
mysqlx.help('getSession');

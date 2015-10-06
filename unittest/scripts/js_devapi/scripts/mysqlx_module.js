var mysqlx = require('mysqlx').mysqlx;

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

print('getSession:', typeof mysqlx.getSession);
print('getNodeSession:', typeof mysqlx.getNodeSession);
print('expr:', typeof mysqlx.expr);


//@ mysqlx module: getSession through URI
var mySession = mysqlx.getSession(__uripwd);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');
	
mySession.close();
	
//@ mysqlx module: getSession through URI and password
mySession = mysqlx.getSession(__uri, __pwd);

print(mySession, '\n');

if (mySession.uri == __uri)
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

if (mySession.uri == __uri)
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

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	
	
mySession.close();		


//@ mysqlx module: getNodeSession through URI
mySession = mysqlx.getNodeSession(__uripwd);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');
	
mySession.close();
	
//@ mysqlx module: getNodeSession through URI and password
mySession = mysqlx.getNodeSession(__uri, __pwd);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	

mySession.close();


//@ mysqlx module: getNodeSession through data
var data = { host: __host,
						 port: __port,
						 schema: __schema,
						 dbUser: __user,
						 dbPassword: __pwd };


mySession = mysqlx.getNodeSession(data);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	
	
mySession.close();		

//@ mysqlx module: getNodeSession through data and password
var data = { host: __host,
						 port: __port,
						 schema: __schema,
						 dbUser: __user};


mySession = mysqlx.getNodeSession(data, __pwd);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	
	
mySession.close();		



//@# mysqlx module: expression errors
var expr;
expr = mysqlx.expr();
expr = mysqlx.expr(5);

//@ mysqlx module: expression
expr = mysqlx.expr('5+6');
print(expr);
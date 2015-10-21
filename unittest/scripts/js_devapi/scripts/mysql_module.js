var mysql = require('mysql').mysql;

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

//@ mysql module: getClassicSession through URI
mySession = mysql.getClassicSession(__uripwd);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');
	
mySession.close();
	
//@ mysql module: getClassicSession through URI and password
mySession = mysql.getClassicSession(__uri, __pwd);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	

mySession.close();


//@ mysql module: getClassicSession through data
var data = { host: __host,
						 port: __port,
						 schema: __schema,
						 dbUser: __user,
						 dbPassword: __pwd };


mySession = mysql.getClassicSession(data);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	
	
mySession.close();		

//@ mysql module: getClassicSession through data and password
var data = { host: __host,
						 port: __port,
						 schema: __schema,
						 dbUser: __user};


mySession = mysql.getClassicSession(data, __pwd);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	
	
mySession.close();

//@ Server Registry, session from data dictionary
shell.registry.store('mysql_data', data);

mySession = mysql.getClassicSession(shell.registry.mysql_data, __pwd);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	

mySession.close();

//@ Server Registry, session from data dictionary removed
shell.registry.remove('mysql_data');
mySession = mysql.getClassicSession(shell.registry.mysql_data, __pwd);


//@ Server Registry, session from uri
shell.registry.store('mysql_uri', __uripwd);

mySession = mysql.getClassicSession(shell.registry.mysql_uri);

print(mySession, '\n');

if (mySession.uri == __uri)
	print('Session using right URI\n');
else
	print('Session using wrong URI\n');	

mySession.close();

//@ Server Registry, session from uri removed
shell.registry.remove('mysql_uri');
mySession = mysql.getClassicSession(shell.registry.mysql_uri);


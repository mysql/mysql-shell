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

//@ mysql module: getClassicSession through URI
mySession = mysql.getClassicSession(__uripwd);

print(mySession, '\n');

if (mySession.uri == __displayuri)
  print('Session using right URI\n');
else
  print('Session using wrong URI\n');

mySession.close();

//@ mysql module: getClassicSession through URI and password
mySession = mysql.getClassicSession(__uri, __pwd);

print(mySession, '\n');

if (mySession.uri == __displayuri)
  print('Session using right URI\n');
else
  print('Session using wrong URI\n');

mySession.close();

//@ mysql module: getClassicSession through data
var data = {
  host: __host,
  port: __port,
  schema: __schema,
  dbUser: __user,
  dbPassword: __pwd
};

mySession = mysql.getClassicSession(data);

print(mySession, '\n');
print(mySession.uri, '\n');
print(__displayuridb, '\n');

if (mySession.uri == __displayuridb)
  print('Session using right URI\n');
else
  print('Session using wrong URI\n');

mySession.close();

//@ mysql module: getClassicSession through data and password
var data = {
  host: __host,
  port: __port,
  schema: __schema,
  dbUser: __user
};

mySession = mysql.getClassicSession(data, __pwd);

print(mySession, '\n');

if (mySession.uri == __displayuridb)
  print('Session using right URI\n');
else
  print('Session using wrong URI\n');

//@ mysql module: getClassicSession using URI with duplicated parameters
var mySslSession = mysql.getClassicSession(mySession.uri + "?ssl-mode=REQUIRED&ssl-mode=DISABLED");

//@ mysql module: getClassicSession using dictionary with duplicated parameters
var data = {host: 'localhost', port: 4520, user: 'root', password: 'pwd', 'Ssl-Mode':"DISABLED", 'ssl-mode':'REQUIRED'}
var mySslSession = mysql.getClassicSession(data)

//@ mysql module: getClassicSession using SSL in URI
var res = mySession.runSql('select @@have_ssl');
var row = res.fetchOne();
var have_ssl = (row[0] == 'YES');

var uri_value = 'DISABLED';
if (have_ssl)
  ssl_mode = 'REQUIRED';

var mySslSession = mysql.getClassicSession(mySession.uri + "?ssl-mode=" + ssl_mode);

if (mySslSession.uri == (__displayuridb + "?ssl-mode=" + ssl_mode))
  print('Session using right SSL URI\n');
else
  print('Session using wrong SSL URI\n');


mySslSession.close();
mySession.close();

//@<OUT> Help on getClassicSession
mysql.help('getClassicSession');

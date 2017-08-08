from mysqlsh import mysql

# The tests assume the next variables have been put in place
# on the JS Context
# __uri: <user>@<host>
# __host: <host>
# __port: <port>
# __user: <user>
# __uripwd: <uri>:<pwd>@<host>
# __pwd: <pwd>


#@ mysql module: exports
all_exports = dir(mysql)

# Remove the python built in members
exports = []
for member in all_exports:
  if not member.startswith('__'):
    exports.append(member)


# The dir function appends 3 built in members
print 'Exported Items:', len(exports)

print 'get_classic_session:', type(mysql.get_classic_session)
print 'help:', type(mysql.get_classic_session)

#@ mysql module: get_classic_session through URI
mySession = mysql.get_classic_session(__uripwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysql module: get_classic_session through URI and password
mySession = mysql.get_classic_session(__uri, __pwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()


#@ mysql module: get_classic_session through data
data = { 'host': __host,
						 'port': __port,
						 'schema': __schema,
						 'dbUser': __user,
						 'dbPassword': __pwd }


mySession = mysql.get_classic_session(data)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysql module: get_classic_session through data and password
data = { 'host': __host,
						 'port': __port,
						 'schema': __schema,
						 'dbUser': __user}


mySession = mysql.get_classic_session(data, __pwd)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

#@ mysql module: get_classic_session using URI with duplicated parameters
mySslSession = mysql.get_classic_session(mySession.uri + "?ssl-mode=REQUIRED&ssl-mode=DISABLED")

#@ mysql module: get_classic_session using dictionary with duplicated parameters
data = {'host': 'localhost', 'port': 4520, 'user': 'root', 'password': 'pwd', 'Ssl-Mode':"DISABLED", 'ssl-mode':'REQUIRED'}
mySslSession = mysql.get_classic_session(data)

#@ mysql module: get_classic_session using SSL in URI
res = mySession.run_sql('select @@have_ssl')
row = res.fetch_one()
have_ssl = (row[0] == 'YES');

uri_value = 'DISABLED';
if have_ssl:
  ssl_mode = 'REQUIRED';

mySslSession = mysql.get_classic_session(mySession.uri + "?ssl-mode=" + ssl_mode);

if mySslSession.uri == (__displayuridb + "?ssl-mode=" + ssl_mode):
  print 'Session using right SSL URI\n'
else:
  print 'Session using wrong SSL URI\n'

mySslSession.close();
mySession.close()

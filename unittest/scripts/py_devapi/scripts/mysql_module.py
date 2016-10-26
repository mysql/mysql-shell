import mysql

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

mySession.close()

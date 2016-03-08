import mysqlx

# The tests assume the next variables have been put in place
# on the JS Context
# __uri: <user>@<host>
# __host: <host>
# __port: <port>
# __user: <user>
# __uripwd: <uri>:<pwd>@<host>
# __pwd: <pwd>


#@ mysqlx module: exports
all_members = dir(mysqlx)

# Remove the python built in members
exports = []
for member in all_members:
  if not member.startswith('__'):
    exports.append(member)

print 'Exported Items:', len(exports)

print 'getSession:', type(mysqlx.getSession), '\n'
print 'getNodeSession:', type(mysqlx.getNodeSession), '\n'
print 'expr:', type(mysqlx.expr), '\n'
print 'Type:', mysqlx.Type, '\n'
print 'IndexType:', mysqlx.IndexType, '\n'

#@ mysqlx module: getSession through URI
mySession = mysqlx.getSession(__uripwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysqlx module: getSession through URI and password
mySession = mysqlx.getSession(__uri, __pwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()


#@ mysqlx module: getSession through data
data = { 'host': __host,
				 'port': __port,
				 'schema': __schema,
				 'dbUser': __user,
				 'dbPassword': __pwd }


mySession = mysqlx.getSession(data)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysqlx module: getSession through data and password
data = { 'host': __host,
				 'port': __port,
				 'schema': __schema,
				 'dbUser': __user}


mySession = mysqlx.getSession(data, __pwd)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()


#@ mysqlx module: getNodeSession through URI
mySession = mysqlx.getNodeSession(__uripwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysqlx module: getNodeSession through URI and password
mySession = mysqlx.getNodeSession(__uri, __pwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()


#@ mysqlx module: getNodeSession through data
data = { 'host': __host,
				 'port': __port,
				 'schema': __schema,
				 'dbUser': __user,
				 'dbPassword': __pwd }


mySession = mysqlx.getNodeSession(data)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysqlx module: getNodeSession through data and password
data = { 'host': __host,
				 'port': __port,
				 'schema': __schema,
				 'dbUser': __user}


mySession = mysqlx.getNodeSession(data, __pwd)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()


#@ Stored Sessions, session from data dictionary
shell.storedSessions.add('mysqlx_data', data);

mySession = mysqlx.getSession(shell.storedSessions.mysqlx_data, __pwd);

print "%s\n" % mySession

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'	

mySession.close()

#@ Stored Sessions, session from data dictionary removed
shell.storedSessions.remove('mysqlx_data')
mySession = mysqlx.getSession(shell.storedSessions.mysqlx_data)


#@ Stored Sessions, session from uri
shell.storedSessions.add('mysqlx_uri', __uripwd)

mySession = mysqlx.getSession(shell.storedSessions.mysqlx_uri, __pwd)

print "%s\n" % mySession

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ Stored Sessions, session from uri removed
shell.storedSessions.remove('mysqlx_uri')
mySession = mysqlx.getSession(shell.storedSessions.mysqlx_uri)


# @# mysqlx module: expression errors
# expr = mysqlx.expr()
# expr = mysqlx.expr(5)

#@ mysqlx module: expression
expr = mysqlx.expr('5+6')
print expr

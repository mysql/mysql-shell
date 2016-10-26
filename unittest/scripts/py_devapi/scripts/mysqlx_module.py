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

print 'get_session:', type(mysqlx.get_session), '\n'
print 'get_node_session:', type(mysqlx.get_node_session), '\n'
print 'expr:', type(mysqlx.expr), '\n'
print 'dateValue:', type(mysqlx.date_value), '\n'
print 'help:', type(mysqlx.date_value), '\n'
print 'Type:', mysqlx.Type, '\n'
print 'IndexType:', mysqlx.IndexType, '\n'

#@ mysqlx module: get_session through URI
mySession = mysqlx.get_session(__uripwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysqlx module: get_session through URI and password
mySession = mysqlx.get_session(__uri, __pwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()


#@ mysqlx module: get_session through data
data = { 'host': __host,
				 'port': __port,
				 'schema': __schema,
				 'dbUser': __user,
				 'dbPassword': __pwd }


mySession = mysqlx.get_session(data)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysqlx module: get_session through data and password
data = { 'host': __host,
				 'port': __port,
				 'schema': __schema,
				 'dbUser': __user}


mySession = mysqlx.get_session(data, __pwd)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()


#@ mysqlx module: get_node_session through URI
mySession = mysqlx.get_node_session(__uripwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysqlx module: get_node_session through URI and password
mySession = mysqlx.get_node_session(__uri, __pwd)

print mySession, '\n'

if mySession.uri == __displayuri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()


#@ mysqlx module: get_node_session through data
data = { 'host': __host,
				 'port': __port,
				 'schema': __schema,
				 'dbUser': __user,
				 'dbPassword': __pwd }


mySession = mysqlx.get_node_session(data)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

#@ mysqlx module: get_node_session through data and password
data = { 'host': __host,
				 'port': __port,
				 'schema': __schema,
				 'dbUser': __user}


mySession = mysqlx.get_node_session(data, __pwd)

print mySession, '\n'

if mySession.uri == __displayuridb:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()

# @# mysqlx module: expression errors
# expr = mysqlx.expr()
# expr = mysqlx.expr(5)

#@ mysqlx module: expression
expr = mysqlx.expr('5+6')
print expr

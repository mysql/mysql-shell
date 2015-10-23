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
print 'Decimal:', type(mysqlx.Decimal), '\n'
print 'Numeric:', type(mysqlx.Numeric), '\n'
print mysqlx.TinyInt, '\n'
print mysqlx.SmallInt, '\n'
print mysqlx.MediumInt, '\n'
print mysqlx.Int, '\n'
print mysqlx.Integer, '\n'
print mysqlx.BigInt, '\n'
print mysqlx.Real, '\n'
print mysqlx.Float, '\n'
print mysqlx.Double, '\n'
print mysqlx.Numeric, '\n'
print mysqlx.Date, '\n'
print mysqlx.Time, '\n'
print mysqlx.Timestamp, '\n'
print mysqlx.DateTime, '\n'
print mysqlx.Year, '\n'
print mysqlx.Bit, '\n'
print mysqlx.Blob, '\n'
print mysqlx.Text, '\n'
print mysqlx.IndexUnique, '\n'
print mysqlx.Decimal(), '\n'
print mysqlx.Decimal(10), '\n'
print mysqlx.Decimal(10, 3), '\n'
print mysqlx.Numeric(), '\n'
print mysqlx.Numeric(10), '\n'
print mysqlx.Numeric(10, 3), '\n'

#@ mysqlx module: getSession through URI
mySession = mysqlx.getSession(__uripwd)

print mySession, '\n'

if mySession.uri == __uri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'
	
mySession.close()
	
#@ mysqlx module: getSession through URI and password
mySession = mysqlx.getSession(__uri, __pwd)

print mySession, '\n'

if mySession.uri == __uri:
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

if mySession.uri == __uri:
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

if mySession.uri == __uri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()		


#@ mysqlx module: getNodeSession through URI
mySession = mysqlx.getNodeSession(__uripwd)

print mySession, '\n'

if mySession.uri == __uri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'
	
mySession.close()
	
#@ mysqlx module: getNodeSession through URI and password
mySession = mysqlx.getNodeSession(__uri, __pwd)

print mySession, '\n'

if mySession.uri == __uri:
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

if mySession.uri == __uri:
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

if mySession.uri == __uri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'
	
mySession.close()		


#@ Server Registry, session from data dictionary
shell.registry.add('mysqlx_data', data);

mySession = mysqlx.getSession(shell.registry.mysqlx_data, __pwd);

print "%s\n" % mySession

if mySession.uri == __uri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'	

mySession.close()
	
#@ Server Registry, session from data dictionary removed
shell.registry.remove('mysqlx_data')
mySession = mysqlx.getSession(shell.registry.mysqlx_data, __pwd)


#@ Server Registry, session from uri
shell.registry.add('mysqlx_uri', __uripwd)

mySession = mysqlx.getSession(shell.registry.mysqlx_uri)

print "%s\n" % mySession

if mySession.uri == __uri:
	print 'Session using right URI\n'
else:
	print 'Session using wrong URI\n'

mySession.close()
	
#@ Server Registry, session from uri removed
shell.registry.remove('mysqlx_uri')
mySession = mysqlx.getSession(shell.registry.mysqlx_uri)


# @# mysqlx module: expression errors
# expr = mysqlx.expr()
# expr = mysqlx.expr(5)

#@ mysqlx module: expression
expr = mysqlx.expr('5+6')
print expr
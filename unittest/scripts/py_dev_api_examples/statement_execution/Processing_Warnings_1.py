import mysqlx

def process_warnings(result):
  if result.getWarningCount():
    for warning in result.getWarnings():
      print 'Type [%s] (Code %s): %s\n' % (warning.Level, warning.Code, warning.Message)
  else:
    print "No warnings were returned.\n"
  

# Connect to server
mySession = mysqlx.getNodeSession( {
  'host': 'localhost', 'port': 33060,
  'dbUser': 'mike', 'dbPassword': 's3cr3t!' } );

# Disables warning generation
mySession.setFetchWarnings(False)
result = mySession.sql('drop schema if exists unexisting').execute()
process_warnings(result)

# Enables warning generation
mySession.setFetchWarnings(True)
result = mySession.sql('drop schema if exists unexisting').execute()
process_warnings(result)

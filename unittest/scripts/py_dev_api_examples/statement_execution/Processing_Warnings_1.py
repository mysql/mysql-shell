from mysqlsh import mysqlx

def process_warnings(result):
  if result.get_warning_count():
    for warning in result.get_warnings():
      print 'Type [%s] (Code %s): %s\n' % (warning.level, warning.code, warning.message)
  else:
    print "No warnings were returned.\n"


# Connect to server
mySession = mysqlx.get_session( {
  'host': 'localhost', 'port': 33060,
  'user': 'mike', 'password': 'paSSw0rd' } );

# Disables warning generation
mySession.set_fetch_warnings(False)
result = mySession.sql('drop schema if exists unexisting').execute()
process_warnings(result)

# Enables warning generation
mySession.set_fetch_warnings(True)
result = mySession.sql('drop schema if exists unexisting').execute()
process_warnings(result)

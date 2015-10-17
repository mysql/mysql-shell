# Assumptions: test schema exists
# Passing the paramaters in the { param: value } format
# Query the use for the user information
print "Please enter the database user information."
usr = shell.prompt("Username: ", {'defaultValue': "mike"})
pwd = shell.prompt("Password: ", {'type': "password"})

# Connect to MySQL X server on network machine
mySession = mysqlx.getSession( {
  'host': 'localhost', 'port': 33060,
  'dbUser': usr, 'dbPassword': pwd} )

myDb = mySession.getSchema('test')
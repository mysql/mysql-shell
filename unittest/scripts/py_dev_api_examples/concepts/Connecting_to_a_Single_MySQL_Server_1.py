# Passing the parameters in the { param: value } format
# Query the user for the account information
print("Please enter the database user information.")
usr = shell.prompt("Username: ", {'defaultValue': "mike"})
pwd = shell.prompt("Password: ", {'type': "password"})

# Connect to MySQL Server on a network machine
mySession = mysqlx.get_session( {
        'host': 'localhost', 'port': 33060,
        'user': usr, 'password': pwd} )

myDb = mySession.get_schema('test')

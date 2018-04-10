// Passing the parameters in the { param: value } format
// Query the user for the account information
print("Please enter the database user information.");
var usr = shell.prompt("Username: ", {defaultValue: "mike"});
var pwd = shell.prompt("Password: ", {type: "password"});

// Connect to MySQL Server on a network machine
mySession = mysqlx.getSession( {
        host: 'localhost', 'port': 33060,
        user: usr, password: pwd} );

myDb = mySession.getSchema('test');

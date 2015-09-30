// Assumptions: test schema exists
// Passing the paramaters in the { param: value } format
// Query the use for the user information
print("Please enter the database user information.");
var usr = prompt("Username: ", {defaultValue: "mike"});
var pwd = prompt("Password: ", {type: "password"});

// Connect to MySQL X server on network machine
var mySession = mysqlx.getSession( {
  host: 'localhost', port: 33060,
  dbUser: usr, dbPassword: pwd} );
	
var	myDb = mySession.getSchema('test');
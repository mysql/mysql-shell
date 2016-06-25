// Connecting to MySQL and working with a Session
var mysqlx = require('mysqlx').mysqlx;

// Connect to a dedicated MySQL server using a connection URL
var mySession = mysqlx.getSession('mike:s3cr3t!@localhost');

// Get a list of all available schemas
var schemaList = mySession.getSchemas();

print('Available schemas in this session:\n');

// Loop over all available schemas and print their name
for (index in schemaList) {
  print(schemaList[index].name + '\n');
}

mySession.close();

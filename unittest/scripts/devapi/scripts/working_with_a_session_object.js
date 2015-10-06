// Connecting to MySQL X and working with a Session
var mysqlx = require('mysqlx').mysqlx;

// Connect to a dedicated MySQL X server node using a connection URL
var mySession = mysqlx.getSession('mike:s3cr3t!@localhost');

// Get a list of all available schemas
var schemaList = mySession.getSchemas();

print('Available schemas in this session:\n');

// Loop over all available schemas and print their name
for (schema in schemaList) {
        print(schema + '\n');
}

mySession.close();
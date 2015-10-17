// Assumptions: test schema exists
var mysqlx = require('mysqlx').mysqlx;

var session = mysqlx.getNodeSession({
  dataSourceFile: 'mysqlxconfig.json', app: 'myapp',
  dbUser: 'mike', dbPassword: 's3cr3t!'});

var default_schema = session.getDefaultSchema().name;
session.setCurrentSchema(default_schema);

// print the current schema name
print(session.getCurrentSchema().name);
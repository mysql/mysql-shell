// Assumptions: test schema exists
var mysqlx = require('mysqlx').mysqlx;

var session = mysqlx.getSession({
dataSourceFile: 'mysqlxconfig.json', app: 'myapp',
dbUser: 'mike', dbPassword: 's3cr3t!'});

var schema = session.getDefaultSchema();

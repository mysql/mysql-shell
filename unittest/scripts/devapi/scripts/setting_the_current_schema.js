// Assumptions: test schema exists

var mysqlx = require('mysqlx').mysqlx;

// Direct connect with no client side default schema defined
var mySession = mysqlx.getNodeSession('mike:s3cr3t!@localhost');
mySession.setCurrentSchema("test");
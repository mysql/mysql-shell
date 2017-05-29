
var mysqlx = require('mysqlx');

// Direct connect with no client side default schema defined
var mySession = mysqlx.getSession('mike:s3cr3t!@localhost');
mySession.setCurrentSchema("test");

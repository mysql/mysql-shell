
var mysqlx = require('mysqlx');

// Direct connect with no client side default schema defined
var mySession = mysqlx.getSession('mike:paSSw0rd@localhost');
mySession.setCurrentSchema("test");

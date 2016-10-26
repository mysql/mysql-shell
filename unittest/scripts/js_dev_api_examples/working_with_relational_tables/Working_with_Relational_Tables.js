
// Working with Relational Tables
var mysqlx = require('mysqlx');

// Connect to server using a connection URL
var mySession = mysqlx.getNodeSession( {
  host: 'localhost', port: 33060,
  dbUser: 'mike', dbPassword: 's3cr3t!'} )
  
var myDb = mySession.getSchema('test');

// Accessing an existing table
var myTable = myDb.getTable('my_table');

// Insert SQL Table data
myTable.insert(['name', 'birthday', 'age']).
  values('Sakila', mysqlx.dateValue(2000, 5, 27), 16).execute();

// Find a row in the SQL Table
var myResult = myTable.select(['_id', 'name', 'birthday']).
  where('name like :name AND age < :age').
  bind('name', 'S%').bind('age', 20).execute();

// Print result
print(myResult.fetchOne());


// Working with Relational Tables
var mysqlx = require('mysqlx').mysqlx;

// Connect to server using a connection URL
var db = mysqlx.getSession( {
  host: 'localhost', port: 33060,
  dbUser: 'mike', dbPassword: 's3cr3t!'} )
  .getSchema('test');

// Accessing an existing table
var myTable = db.getTable('my_table');

// Insert SQL Table data
myTable.insert({
  name: 'Sakila',
  birthday: mysqlx.dateValue(2000, 5, 27),
  age: 16}).execute();

// Find a row in the SQL Table
var myResult = myTable.select(['_id', 'name', 'birthday']).
  where('name like :name AND age < :age').
  bind('name', 'S%').bind('age', 20).execute();

// Print result
print(myResult.fetchOne());

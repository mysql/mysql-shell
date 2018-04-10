
var mysqlx = require('mysqlx');

// Connect to server
var mySession = mysqlx.getSession( {
        host: 'localhost', port: 33060,
        user: 'mike', password: 'paSSw0rd' } );

// Get the Schema test
var myDb = mySession.getSchema('test');

// Create a new collection
var myColl = myDb.createCollection('my_collection');

// Start a transaction
mySession.startTransaction();
try
{
    myColl.add({'name': 'Jack', 'age': 15, 'height': 1.76, 'weight': 69.4}).execute();
    myColl.add({'name': 'Susanne', 'age': 24, 'height': 1.65}).execute();
    myColl.add({'name': 'Mike', 'age': 39, 'height': 1.9, 'weight': 74.3}).execute();

    // Commit the transaction if everything went well
    var reply = mySession.commit();

    // handle warnings
    if (reply.warningCount){
      var warnings = reply.getWarnings();
      for (index in warnings){
        var warning = warnings[index];
        print ('Type ['+ warning.level + '] (Code ' + warning.code + '): ' + warning.message + '\n');
      }
    }

    print ('Data inserted successfully.');
}
catch(err)
{
    // Rollback the transaction in case of an error
    reply = mySession.rollback();

    // handle warnings
    if (reply.warningCount){
      var warnings = reply.getWarnings();
      for (index in warnings){
        var warning = warnings[index];
        print ('Type ['+ warning.level + '] (Code ' + warning.code + '): ' + warning.message + '\n');
      }
    }

    // Printing the error message
    print ('Data could not be inserted: ' + err.message);
}

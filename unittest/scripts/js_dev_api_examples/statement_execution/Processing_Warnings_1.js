
var mysqlx = require('mysqlx');

function process_warnings(result){
  if (result.getWarningCount()){
    var warnings = result.getWarnings();
    for (index in warnings){
      var warning = warnings[index];
      print ('Type ['+ warning.level + '] (Code ' + warning.code + '): ' + warning.message + '\n');
    }
  }
  else{
    print ("No warnings were returned.\n");
  }
}

// Connect to server
var mySession = mysqlx.getSession( {
  host: 'localhost', port: 33060,
  user: 'mike', password: 'paSSw0rd' } );

// Disables warning generation
mySession.setFetchWarnings(false);
var result = mySession.sql('drop schema if exists unexisting').execute();
process_warnings(result);

// Enables warning generation
mySession.setFetchWarnings(true);
var result = mySession.sql('drop schema if exists unexisting').execute();
process_warnings(result);

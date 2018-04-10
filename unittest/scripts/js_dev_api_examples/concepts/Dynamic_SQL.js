
function createTestTable(session, name) {

  // use escape function to quote names/identifier
  quoted_name = session.quoteName(name);

  session.sql("DROP TABLE IF EXISTS " + quoted_name).execute();

  var create = "CREATE TABLE ";
  create += quoted_name;
  create += " (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT)";

  session.sql(create).execute();

  return session.getCurrentSchema().getTable(name);
}

var mysqlx = require('mysqlx');

var session = mysqlx.getSession('mike:paSSw0rd@localhost:33060/test');

var default_schema = session.getDefaultSchema().name;
session.setCurrentSchema(default_schema);

// Creates some tables
var table1 = createTestTable(session, 'test1');
var table2 = createTestTable(session, 'test2');

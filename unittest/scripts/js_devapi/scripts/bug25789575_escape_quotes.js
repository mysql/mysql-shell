// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'string_quote_escape');

var schema = mySession.createSchema('string_quote_escape');

// Create collection
var collection = schema.createCollection('bug25789575');

//@ Add data to collection
collection.add({"_id": "1", "a": 1});
collection.add({"_id": "2", "key'with'single'quotes": "value'with'single'quotes"})
collection.add({"_id": "3", "key\"with\"double\"quotes": "value\"with\"double\"quotes"})
collection.add({"_id": "4", "key\"with\"mixed'quo'\"tes": "value\"with'mixed\"quotes''\""})
collection.add({"_id": "5", "utf8": "I ❤ MySQL Shell"})
collection.add({"_id": "6", "new_line": "\\n is a newline\nthis is second line."})
collection.add({"_id": "7", "tab_stop": "\\t is a tab stop\n\tnewline with tab stop."})

//@<OUT> find()
collection.find()

//@<OUT> find().execute().fetchAll()
collection.find().execute().fetchAll()

//@<OUT> get as table
t = schema.getCollectionAsTable("bug25789575")

//@<OUT> select()
t.select()

//@<OUT> execute().fetchAll()
t.select().execute().fetchAll()

//@<OUT> Cleanup
mySession.dropSchema('string_quote_escape');
mySession.close();
print("o/")
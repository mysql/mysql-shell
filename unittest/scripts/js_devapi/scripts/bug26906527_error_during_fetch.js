// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'error_during_fetch');

var schema = mySession.createSchema('error_during_fetch');

// Create collection
var collection = schema.createCollection('test');

//@ Add data to collection
collection.add({_id: '3C514FF38144B714E7119BCF48B4CA01'});
collection.add({_id: '3C514FF38144B714E7119BCF48B4CA02'});
collection.add({_id: '3C514FF38144B714E7119BCF48B4CA03'});
collection.add({_id: '3C514FF38144B714E7119BCF48B4CA04'});
collection.add({_id: '3C514FF38144B714E7119BCF48B4CA05'});
collection.add({_id: '3C514FF38144B714E7119BCF48B4CA06', "name":"foo"});

//@ find().fields()
collection.find().fields("name");
collection.find().fields(["json_type(name)"]);

//@<OUT> Cleanup
mySession.dropSchema('error_during_fetch');
mySession.close();
print("o/")

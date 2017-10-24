// Assumptions: validate_crud_functions available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

schema = mySession.createSchema('js_shell_test');

// Creates a test collection and inserts data into it
var col = schema.createCollection('collection1');

//@<OUT> Collection.help()
col.help()

//@<OUT> Collection.help('add')
col.help('add')

//@<OUT> Collection.help('addOrReplaceOne')
col.help('addOrReplaceOne')

//@<OUT> Collection.help('createIndex')
col.help('createIndex')

//@<OUT> Collection.help('dropIndex')
col.help('dropIndex')

//@<OUT> Collection.help('existsInDatabase')
col.help('existsInDatabase')

//@<OUT> Collection.help('find')
col.help('find')

//@<OUT> Collection.help('getName')
col.help('getName')

//@<OUT> Collection.help('getOne')
col.help('getOne')

//@<OUT> Collection.help('getSchema')
col.help('getSchema')

//@<OUT> Collection.help('getSession')
col.help('getSession')

//@<OUT> Collection.help('modify')
col.help('modify')

//@<OUT> Collection.help('remove')
col.help('remove')

//@<OUT> Collection.help('removeOne')
col.help('removeOne')

//@<OUT> Collection.help('replaceOne')
col.help('replaceOne')

//@ Finalization
mySession.dropSchema('js_shell_test');

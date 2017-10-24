# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
mysqlx = require('mysqlx');

mySession = mysqlx.get_session(__uripwd);

ensure_schema_does_not_exist(mySession, 'py_shell_test');

schema = mySession.create_schema('py_shell_test');

# Creates a test collection and inserts data into it
col = schema.create_collection('collection1');

#@<OUT> Collection.help()
col.help()

#@<OUT> Collection.help('add')
col.help('add')

#@<OUT> Collection.help('add_or_replace_one')
col.help('add_or_replace_one')

#@<OUT> Collection.help('create_index')
col.help('create_index')

#@<OUT> Collection.help('drop_index')
col.help('drop_index')

#@<OUT> Collection.help('exists_in_database')
col.help('exists_in_database')

#@<OUT> Collection.help('find')
col.help('find')

#@<OUT> Collection.help('get_name')
col.help('get_name')

#@<OUT> Collection.help('get_one')
col.help('get_one')

#@<OUT> Collection.help('get_schema')
col.help('get_schema')

#@<OUT> Collection.help('get_session')
col.help('get_session')

#@<OUT> Collection.help('modify')
col.help('modify')

#@<OUT> Collection.help('remove')
col.help('remove')

#@<OUT> Collection.help('remove_one')
col.help('remove_one')

#@<OUT> Collection.help('replace_one')
col.help('replace_one')

#@ Finalization
mySession.drop_schema('py_shell_test');

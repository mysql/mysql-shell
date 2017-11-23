# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
# validateMemer and validateNotMember are defined on the setup script
mySession = mysqlx.get_session(__uripwd);

#@<OUT> Session: help
mySession.help();

#@<OUT> Session: help close
mySession.help('close');

#@<OUT> Session: help commit
mySession.help('commit');

#@<OUT> Session: help create_schema
mySession.help('create_schema');

#@<OUT> Session: help drop-schema
mySession.help('drop_schema');

#@<OUT> Session: help get_current_schema
mySession.help('get_current_schema');

#@<OUT> Session: help get_default_schema
mySession.help('get_default_schema');

#@<OUT> Session: help get_schema
mySession.help('get_schema');

#@<OUT> Session: help get_schemas
mySession.help('get_schemas');

#@<OUT> Session: help get_uri
mySession.help('get_uri');

#@<OUT> Session: help is_open
mySession.help('is_open');

#@<OUT> Session: help quote_name
mySession.help('quote_name');

#@<OUT> Session: release_savepoint
mySession.help('release_savepoint');

#@<OUT> Session: help rollback
mySession.help('rollback');

#@<OUT> Session: help set_current_schema
mySession.help('set_current_schema');

#@<OUT> Session: help set_fetch_warnings
mySession.help('set_fetch_warnings');

#@<OUT> Session: help set_savepoint
mySession.help('set_savepoint');

#@<OUT> Session: help sql
mySession.help('sql');

#@<OUT> Session: help start_transaction
mySession.help('start_transaction');

# Cleanup
mySession.close();

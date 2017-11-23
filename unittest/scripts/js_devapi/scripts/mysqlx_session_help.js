// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
// validateMemer and validateNotMember are defined on the setup script
var mysqlx = require('mysqlx');
var mySession = mysqlx.getSession(__uripwd);

//@<OUT> Session: help
mySession.help();

//@<OUT> Session: help close
mySession.help('close');

//@<OUT> Session: help commit
mySession.help('commit');

//@<OUT> Session: help createSchema
mySession.help('createSchema');

//@<OUT> Session: help dropSchema
mySession.help('dropSchema');

//@<OUT> Session: help getCurrentSchema
mySession.help('getCurrentSchema');

//@<OUT> Session: help getDefaultSchema
mySession.help('getDefaultSchema');

//@<OUT> Session: help getSchema
mySession.help('getSchema');

//@<OUT> Session: help getSchemas
mySession.help('getSchemas');

//@<OUT> Session: help getUri
mySession.help('getUri');

//@<OUT> Session: help isOpen
mySession.help('isOpen');

//@<OUT> Session: help quoteName
mySession.help('quoteName');

//@<OUT> Session: releaseSavepoint
mySession.help('releaseSavepoint');

//@<OUT> Session: help rollback
mySession.help('rollback');

//@<OUT> Session: help setCurrentSchema
mySession.help('setCurrentSchema');

//@<OUT> Session: help setFetchWarnings
mySession.help('setFetchWarnings');

//@<OUT> Session: help setSavepoint
mySession.help('setSavepoint');

//@<OUT> Session: help sql
mySession.help('sql');

//@<OUT> Session: help startTransaction
mySession.help('startTransaction');

// Cleanup
mySession.close();

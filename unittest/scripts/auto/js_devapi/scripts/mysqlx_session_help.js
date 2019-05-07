//@ Initialization
var mySession = mysqlx.getSession(__uripwd + '/mysql');

//@ Session.help
mySession.help();

//@ Session.help, \? [USE:Session.help]
\? mysqlx.Session

//@ Help on currentSchema
mySession.help('currentSchema');

//@ Session currentSchema, \? [USE:Help on currentSchema]
\? Session.currentSchema

//@ Help on defaultSchema
mySession.help('defaultSchema');

//@ Session defaultSchema, \? [USE:Help on defaultSchema]
\? Session.defaultSchema

//@ Help on uri
mySession.help('uri');

//@ Session uri, \? [USE:Help on uri]
\? Session.uri

//@ Help on close
mySession.help('close');

//@ Session close, \? [USE:Help on close]
\? Session.close

//@ Help on commit
mySession.help('commit');

//@ Session commit, \? [USE:Help on commit]
\? Session.commit

//@ Help on createSchema
mySession.help('createSchema');

//@ Session createSchema, \? [USE:Help on createSchema]
\? Session.createSchema

//@ Help on dropSchema
mySession.help('dropSchema');

//@ Session dropSchema, \? [USE:Help on dropSchema]
\? Session.dropSchema

//@ Help on getCurrentSchema
mySession.help('getCurrentSchema');

//@ Session getCurrentSchema, \? [USE:Help on getCurrentSchema]
\? Session.getCurrentSchema

//@ Help on getDefaultSchema
mySession.help('getDefaultSchema');

//@ Session getDefaultSchema, \? [USE:Help on getDefaultSchema]
\? Session.getDefaultSchema

//@ Help on getSchema
mySession.help('getSchema');

//@ Session getSchema, \? [USE:Help on getSchema]
\? Session.getSchema

//@ Help on getSchemas
mySession.help('getSchemas');

//@ Session getSchemas, \? [USE:Help on getSchemas]
\? Session.getSchemas

//@ Help on getUri
mySession.help('getUri');

//@ Session getUri, \? [USE:Help on getUri]
\? Session.getUri

//@ Help on help
mySession.help('help');

//@ Session help, \? [USE:Help on help]
\? Session.help

//@ Help on isOpen
mySession.help('isOpen');

//@ Session isOpen, \? [USE:Help on isOpen]
\? Session.isOpen

//@ Help on quoteName
mySession.help('quoteName');

//@ Session quoteName, \? [USE:Help on quoteName]
\? Session.quoteName

//@ Help on releaseSavePoint
mySession.help('releaseSavePoint');

//@ Session releaseSavePoint, \? [USE:Help on releaseSavePoint]
\? Session.releaseSavePoint

//@ Help on rollBack
mySession.help('rollBack');

//@ Session rollBack, \? [USE:Help on rollBack]
\? Session.rollBack

//@ Help on rollBackTo
mySession.help('rollBackTo');

//@ Session rollBackTo, \? [USE:Help on rollBackTo]
\? Session.rollBackTo

//@ Help on runSql
mySession.help('runSql');

//@ Session runSql, \? [USE:Help on runSql]
\? Session.runSql

//@ Help on setCurrentSchema
mySession.help('setCurrentSchema');

//@ Session setCurrentSchema, \? [USE:Help on setCurrentSchema]
\? Session.setCurrentSchema

//@ Help on setFetchWarnings
mySession.help('setFetchWarnings');

//@ Session setFetchWarnings, \? [USE:Help on setFetchWarnings]
\? Session.setFetchWarnings

//@ Help on setSavePoint
mySession.help('setSavePoint');

//@ Session setSavePoint, \? [USE:Help on setSavePoint]
\? Session.setSavePoint

//@ Help on sql
mySession.help('sql');

//@ Session sql, \? [USE:Help on sql]
\? Session.sql

//@ Help on startTransaction
mySession.help('startTransaction');

//@ Session startTransaction, \? [USE:Help on startTransaction]
\? Session.startTransaction

//@ Finalization
mySession.close();

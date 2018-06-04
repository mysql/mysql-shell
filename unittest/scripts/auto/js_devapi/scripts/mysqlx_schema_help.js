//@ Initialization
shell.connect(__uripwd + '/mysql');
var schema = db;
session.close();

//@ Schema.help
schema.help();

//@ Schema.help, \? [USE:Schema.help]
\? mysqlx.Schema

//@ Help on name
schema.help('name');

//@ Schema name, \? [USE:Help on name]
\? schema.name

//@ Help on schema
schema.help('schema');

//@ Schema schema, \? [USE:Help on schema]
\? schema.schema

//@ Help on session
schema.help('session');

//@ Schema session, \? [USE:Help on session]
\? schema.session

//@ Help on createCollection
schema.help('createCollection');

//@ Schema createCollection, \? [USE:Help on createCollection]
\? schema.createCollection

//@ Help on dropCollection
schema.help('dropCollection');

//@ Schema dropCollection, \? [USE:Help on dropCollection]
\? schema.dropCollection

//@ Help on existsInDatabase
schema.help('existsInDatabase');

//@ Schema existsInDatabase, \? [USE:Help on existsInDatabase]
\? schema.existsInDatabase

//@ Help on getCollection
schema.help('getCollection');

//@ Schema getCollection, \? [USE:Help on getCollection]
\? schema.getCollection

//@ Help on getCollectionAsTable
schema.help('getCollectionAsTable');

//@ Schema getCollectionAsTable, \? [USE:Help on getCollectionAsTable]
\? schema.getCollectionAsTable

//@ Help on getCollections
schema.help('getCollections');

//@ Schema getCollections, \? [USE:Help on getCollections]
\? schema.getCollections

//@ Help on getName
schema.help('getName');

//@ Schema getName, \? [USE:Help on getName]
\? schema.getName

//@ Help on getSchema
schema.help('getSchema');

//@ Schema getSchema, \? [USE:Help on getSchema]
\? schema.getSchema

//@ Help on getSession
schema.help('getSession');

//@ Schema getSession, \? [USE:Help on getSession]
\? schema.getSession

//@ Help on getTable
schema.help('getTable');

//@ Schema getTable, \? [USE:Help on getTable]
\? schema.getTable

//@ Help on getTables
schema.help('getTables');

//@ Schema getTables, \? [USE:Help on getTables]
\? schema.getTables

//@ Help on help
schema.help('help');

//@ Schema help, \? [USE:Help on help]
\? schema.help

//@ Initialization
shell.connect(__uripwd);
var schema = session.createSchema('collection_help');
var collection = schema.createCollection('sample');
session.close();

//@ Collection.help
collection.help();

//@ Collection.help, \? [USE:Collection.help]
\? collection

//@ Help on name
collection.help('name');

//@ Collection name, \? [USE:Help on name]
\? collection.name

//@ Help on schema
collection.help('schema');

//@ Collection schema, \? [USE:Help on schema]
\? collection.schema

//@ Help on session
collection.help('session');

//@ Collection session, \? [USE:Help on session]
\? collection.session

//@ Help on add
collection.help('add');

//@ Collection add, \? [USE:Help on add]
\? collection.add

//@ Help on addOrReplaceOne
collection.help('addOrReplaceOne');

//@ Collection addOrReplaceOne, \? [USE:Help on addOrReplaceOne]
\? collection.addOrReplaceOne

//@<OUT> Help on count
collection.help('count');

//@ Collection count, \? [USE:Help on count]
\? collection.count

//@ Help on createIndex
collection.help('createIndex');

//@ Collection createIndex, \? [USE:Help on createIndex]
\? collection.createIndex

//@ Help on dropIndex
collection.help('dropIndex');

//@ Collection dropIndex, \? [USE:Help on dropIndex]
\? collection.dropIndex

//@ Help on existsInDatabase
collection.help('existsInDatabase');

//@ Collection existsInDatabase, \? [USE:Help on existsInDatabase]
\? collection.existsInDatabase

//@ Help on find
collection.help('find');

//@ Collection find, \? [USE:Help on find]
\? collection.find

//@ Help on getName
collection.help('getName');

//@ Collection getName, \? [USE:Help on getName]
\? collection.getName

//@ Help on getOne
collection.help('getOne');

//@ Collection getOne, \? [USE:Help on getOne]
\? collection.getOne

//@ Help on getSchema
collection.help('getSchema');

//@ Collection getSchema, \? [USE:Help on getSchema]
\? collection.getSchema

//@ Help on getSession
collection.help('getSession');

//@ Collection getSession, \? [USE:Help on getSession]
\? collection.getSession

//@ Help on help
collection.help('help');

//@ Collection help, \? [USE:Help on help]
\? collection.help

//@ Help on modify
collection.help('modify');

//@ Collection modify, \? [USE:Help on modify]
\? collection.modify

//@ Help on remove
collection.help('remove');

//@ Collection remove, \? [USE:Help on remove]
\? collection.remove

//@ Help on removeOne
collection.help('removeOne');

//@ Collection removeOne, \? [USE:Help on removeOne]
\? collection.removeOne

//@ Finalization
shell.connect(__uripwd);
session.dropSchema('collection_help');
session.close();

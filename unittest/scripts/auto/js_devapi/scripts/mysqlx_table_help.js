//@ Initialization
shell.connect(__uripwd + '/mysql');
var table = db.user;
session.close();

//@ Table.help
table.help();

//@ Table.help, \? [USE:Table.help]
\? table

//@ Help on name
table.help('name');

//@ Table name, \? [USE:Help on name]
\? table.name

//@ Help on schema
table.help('schema');

//@ Table schema, \? [USE:Help on schema]
\? table.schema

//@ Help on session
table.help('session');

//@ Table session, \? [USE:Help on session]
\? table.session

//@ Help on delete
table.help('delete');

//@ Table delete, \? [USE:Help on delete]
\? table.delete

//@ Help on existsInDatabase
table.help('existsInDatabase');

//@ Table existsInDatabase, \? [USE:Help on existsInDatabase]
\? table.existsInDatabase

//@ Help on getName
table.help('getName');

//@ Table getName, \? [USE:Help on getName]
\? table.getName

//@ Help on getSchema
table.help('getSchema');

//@ Table getSchema, \? [USE:Help on getSchema]
\? table.getSchema

//@ Help on getSession
table.help('getSession');

//@ Table getSession, \? [USE:Help on getSession]
\? table.getSession

//@ Help on help
table.help('help');

//@ Table help, \? [USE:Help on help]
\? table.help

//@ Help on insert
table.help('insert');

//@ Table insert, \? [USE:Help on insert]
\? table.insert

//@ Help on isView
table.help('isView');

//@ Table isView, \? [USE:Help on isView]
\? table.isView

//@ Help on select
table.help('select');

//@ Table select, \? [USE:Help on select]
\? table.select

//@ Help on update
table.help('update');

//@ Table update, \? [USE:Help on update]
\? table.update

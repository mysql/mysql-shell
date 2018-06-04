//@ Initialization
shell.connect(__uripwd + '/mysql');
var table = db.user;
var crud = table.insert();
session.close();

//@ TableInsert help
crud.help();

//@ TableInsert help, \? [USE:TableInsert help]
\? TableInsert

//@ Help on execute
crud.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? TableInsert.execute

//@ Help on help
crud.help('help');

//@ Help on help, \? [USE:Help on help]
\? TableInsert.help

//@ Help on insert
crud.help('insert');

//@ Help on insert, \? [USE:Help on insert]
\? TableInsert.insert

//@ Help on values
crud.help('values');

//@ Help on values, \? [USE:Help on values]
\? TableInsert.values

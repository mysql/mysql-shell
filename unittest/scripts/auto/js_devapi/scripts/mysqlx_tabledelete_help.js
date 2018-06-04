//@ Initialization
shell.connect(__uripwd + '/mysql');
var table = db.user;
var crud = table.delete();
session.close();

//@ TableDelete help
crud.help();

//@ TableDelete help, \? [USE:TableDelete help]
\? TableDelete

//@ Help on bind
crud.help('bind');

//@ Help on bind, \? [USE:Help on bind]
\? TableDelete.bind

//@ Help on delete
crud.help('delete');

//@ Help on delete, \? [USE:Help on delete]
\? TableDelete.delete

//@ Help on execute
crud.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? TableDelete.execute

//@ Help on help
crud.help('help');

//@ Help on help, \? [USE:Help on help]
\? TableDelete.help

//@ Help on limit
crud.help('limit');

//@ Help on limit, \? [USE:Help on limit]
\? TableDelete.limit

//@ Help on orderBy
crud.help('orderBy');

//@ Help on orderBy, \? [USE:Help on orderBy]
\? TableDelete.orderBy

//@ Help on where
crud.help('where');

//@ Help on where, \? [USE:Help on where]
\? TableDelete.where

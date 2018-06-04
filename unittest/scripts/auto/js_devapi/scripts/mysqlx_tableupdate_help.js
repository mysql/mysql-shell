//@ Initialization
shell.connect(__uripwd + '/mysql');
var table = db.user;
var crud = table.update();
session.close();

//@ TableUpdate help
crud.help();

//@ TableUpdate help, \? [USE:TableUpdate help]
\? TableUpdate

//@ Help on bind
crud.help('bind');

//@ Help on bind, \? [USE:Help on bind]
\? TableUpdate.bind

//@ Help on execute
crud.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? TableUpdate.execute

//@ Help on help
crud.help('help');

//@ Help on help, \? [USE:Help on help]
\? TableUpdate.help

//@ Help on limit
crud.help('limit');

//@ Help on limit, \? [USE:Help on limit]
\? TableUpdate.limit

//@ Help on orderBy
crud.help('orderBy');

//@ Help on orderBy, \? [USE:Help on orderBy]
\? TableUpdate.orderBy

//@ Help on set
crud.help('set');

//@ Help on set, \? [USE:Help on set]
\? TableUpdate.set

//@ Help on where
crud.help('where');

//@ Help on where, \? [USE:Help on where]
\? TableUpdate.where

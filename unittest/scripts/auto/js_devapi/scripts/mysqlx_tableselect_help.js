//@ Initialization
shell.connect(__uripwd + '/mysql');
var table = db.user;
var crud = table.select();
session.close();

//@ TableSelect help
crud.help();

//@ TableSelect help, \? [USE:TableSelect help]
\? TableSelect

//@ Help on bind
crud.help('bind');

//@ Help on bind, \? [USE:Help on bind]
\? TableSelect.bind

//@ Help on execute
crud.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? TableSelect.execute

//@ Help on groupBy
crud.help('groupBy');

//@ Help on groupBy, \? [USE:Help on groupBy]
\? TableSelect.groupBy

//@ Help on having
crud.help('having');

//@ Help on having, \? [USE:Help on having]
\? TableSelect.having

//@ Help on help
crud.help('help');

//@ Help on help, \? [USE:Help on help]
\? TableSelect.help

//@ Help on limit
crud.help('limit');

//@ Help on limit, \? [USE:Help on limit]
\? TableSelect.limit

//@ Help on lockExclusive
crud.help('lockExclusive');

//@ Help on lockExclusive, \? [USE:Help on lockExclusive]
\? TableSelect.lockExclusive

//@ Help on lockShared
crud.help('lockShared');

//@ Help on lockShared, \? [USE:Help on lockShared]
\? TableSelect.lockShared

//@ Help on offset
crud.help('offset');

//@ Help on offset, \? [USE:Help on offset]
\? TableSelect.offset

//@ Help on orderBy
crud.help('orderBy');

//@ Help on orderBy, \? [USE:Help on orderBy]
\? TableSelect.orderBy

//@ Help on select
crud.help('select');

//@ Help on select, \? [USE:Help on select]
\? TableSelect.select

//@ Help on where
crud.help('where');

//@ Help on where, \? [USE:Help on where]
\? TableSelect.where

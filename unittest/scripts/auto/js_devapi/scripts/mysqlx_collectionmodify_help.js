//@ Initialization
shell.connect(__uripwd);
var schema = session.createSchema('collectionmodify_help');
var collection = schema.createCollection('sample');
var crud = collection.modify("true")
session.close();

//@ CollectionModify help
crud.help();

//@ CollectionModify help, \? [USE:CollectionModify help]
\? CollectionModify

//@ Help on arrayAppend
crud.help('arrayAppend');

//@ Help on arrayAppend, \? [USE:Help on arrayAppend]
\? CollectionModify.arrayAppend

//@ Help on arrayDelete
crud.help('arrayDelete');

//@ Help on arrayDelete, \? [USE:Help on arrayDelete]
\? CollectionModify.arrayDelete

//@ Help on arrayInsert
crud.help('arrayInsert');

//@ Help on arrayInsert, \? [USE:Help on arrayInsert]
\? CollectionModify.arrayInsert

//@ Help on bind
crud.help('bind');

//@ Help on bind, \? [USE:Help on bind]
\? CollectionModify.bind

//@ Help on execute
crud.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? CollectionModify.execute

//@ Help on help
crud.help('help');

//@ Help on help, \? [USE:Help on help]
\? CollectionModify.help

//@ Help on limit
crud.help('limit');

//@ Help on limit, \? [USE:Help on limit]
\? CollectionModify.limit

//@ Help on merge
crud.help('merge');

//@ Help on merge, \? [USE:Help on merge]
\? CollectionModify.merge

//@ Help on modify
crud.help('modify');

//@ Help on modify, \? [USE:Help on modify]
\? CollectionModify.modify

//@ Help on patch
crud.help('patch');

//@ Help on patch, \? [USE:Help on patch]
\? CollectionModify.patch

//@ Help on set
crud.help('set');

//@ Help on set, \? [USE:Help on set]
\? CollectionModify.set

//@ Help on sort
crud.help('sort');

//@ Help on sort, \? [USE:Help on sort]
\? CollectionModify.sort

//@ Help on unset
crud.help('unset');

//@ Help on unset, \? [USE:Help on unset]
\? CollectionModify.unset

//@ Finalization
shell.connect(__uripwd);
session.dropSchema('collectionmodify_help');
session.close();

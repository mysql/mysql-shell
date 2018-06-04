//@ Initialization
shell.connect(__uripwd);
var schema = session.createSchema('collectionremove_help');
var collection = schema.createCollection('sample');
var crud = collection.remove("true")
session.close();

//@ CollectionRemove help
crud.help();

//@ CollectionRemove help, \? [USE:CollectionRemove help]
\? CollectionRemove

//@ Help on bind
crud.help('bind');

//@ Help on bind, \? [USE:Help on bind]
\? CollectionRemove.bind

//@ Help on execute
crud.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? CollectionRemove.execute

//@ Help on help
crud.help('help');

//@ Help on help, \? [USE:Help on help]
\? CollectionRemove.help

//@ Help on limit
crud.help('limit');

//@ Help on limit, \? [USE:Help on limit]
\? CollectionRemove.limit

//@ Help on remove
crud.help('remove');

//@ Help on remove, \? [USE:Help on remove]
\? CollectionRemove.remove

//@ Help on sort
crud.help('sort');

//@ Help on sort, \? [USE:Help on sort]
\? CollectionRemove.sort

//@ Finalization
shell.connect(__uripwd);
session.dropSchema('collectionremove_help');
session.close();

//@ Initialization
shell.connect(__uripwd);
var schema = session.createSchema('collectionadd_help');
var collection = schema.createCollection('sample');
var crud = collection.add({one:1})
session.close();

//@ CollectionAdd help
crud.help();

//@ CollectionAdd help, \? [USE:CollectionAdd help]
\? CollectionAdd

//@ Help on add
crud.help('add');

//@ Help on add, \? [USE:Help on add]
\? CollectionAdd.add

//@ Help on execute
crud.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? CollectionAdd.execute

//@ Help on help
crud.help('help');

//@ Help on help, \? [USE:Help on help]
\? CollectionAdd.help

//@ Finalization
shell.connect(__uripwd);
session.dropSchema('collectionadd_help');
session.close();

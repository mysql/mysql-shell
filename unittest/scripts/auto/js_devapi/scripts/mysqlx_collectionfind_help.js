//@ Initialization
shell.connect(__uripwd);
var schema = session.createSchema('collectionfind_help');
var collection = schema.createCollection('sample');
var crud = collection.find()
session.close();

//@ CollectionFind help
crud.help();

//@ CollectionFind help, \? [USE:CollectionFind help]
\? CollectionFind

//@ Help on bind
crud.help('bind');

//@ Help on bind, \? [USE:Help on bind]
\? CollectionFind.bind

//@ Help on execute
crud.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? CollectionFind.execute

//@ Help on fields
crud.help('fields');

//@ Help on fields, \? [USE:Help on fields]
\? CollectionFind.fields

//@ Help on find
crud.help('find');

//@ Help on find, \? [USE:Help on find]
\? CollectionFind.find

//@ Help on groupBy
crud.help('groupBy');

//@ Help on groupBy, \? [USE:Help on groupBy]
\? CollectionFind.groupBy

//@ Help on having
crud.help('having');

//@ Help on having, \? [USE:Help on having]
\? CollectionFind.having

//@ Help on help
crud.help('help');

//@ Help on help, \? [USE:Help on help]
\? CollectionFind.help

//@ Help on limit
crud.help('limit');

//@ Help on limit, \? [USE:Help on limit]
\? CollectionFind.limit

//@ Help on lockExclusive
crud.help('lockExclusive');

//@ Help on lockExclusive, \? [USE:Help on lockExclusive]
\? CollectionFind.lockExclusive

//@ Help on lockShared
crud.help('lockShared');

//@ Help on lockShared, \? [USE:Help on lockShared]
\? CollectionFind.lockShared

//@ Help on offset
crud.help('offset');

//@ Help on offset, \? [USE:Help on offset]
\? CollectionFind.offset

//@ Help on skip
crud.help('skip');

//@ Help on skip, \? [USE:Help on skip]
\? CollectionFind.skip

//@ Help on sort
crud.help('sort');

//@ Help on sort, \? [USE:Help on sort]
\? CollectionFind.sort

//@ Finalization
shell.connect(__uripwd);
session.dropSchema('collectionfind_help');
session.close();

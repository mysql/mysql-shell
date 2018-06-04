shell.connect(__mysqluripwd);
session.close();

//@ ClassicSession help
session.help();

//@ ClassicSession help, \? [USE:ClassicSession help]
\? classicsession

//@ Help on uri
session.help('uri')

//@ Help on uri, \? [USE:Help on uri]
\? classicsession.uri

//@ Help on close
session.help('close')

//@ Help on close, \? [USE:Help on close]
\? classicsession.close

//@ Help on commit
session.help('commit')

//@ Help on commit, \? [USE:Help on commit]
\? classicsession.commit

//@ Help on getUri
session.help('getUri')

//@ Help on getUri, \? [USE:Help on getUri]
\? classicsession.getUri

//@ Help on help
session.help('help')

//@ Help on help, \? [USE:Help on help]
\? classicsession.help

//@ Help on isOpen
session.help('isOpen')

//@ Help on isOpen, \? [USE:Help on isOpen]
\? classicsession.isOpen

//@ Help on query
session.help('query')

//@ Help on query, \? [USE:Help on query]
\? classicsession.query

//@ Help on rollback
session.help('rollback')

//@ Help on rollback, \? [USE:Help on rollback]
\? classicsession.rollback

//@ Help on runSql
session.help('runSql')

//@ Help on runSql, \? [USE:Help on runSql]
\? classicsession.runSql

//@ Help on startTransaction
session.help('startTransaction')

//@ Help on startTransaction, \? [USE:Help on startTransaction]
\? classicsession.startTransaction

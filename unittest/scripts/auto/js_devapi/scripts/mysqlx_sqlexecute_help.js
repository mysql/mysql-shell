//@ Initialization
var mySession = mysqlx.getSession(__uripwd + '/mysql');
var sql = mySession.sql("select 1");

//@ SqlExecute help
sql.help();

//@ SqlExecute help, \? [USE:SqlExecute help]
\? SqlExecute

//@ Help on bind
sql.help('bind');

//@ Help on bind, \? [USE:Help on bind]
\? SqlExecute.bind

//@ Help on execute
sql.help('execute');

//@ Help on execute, \? [USE:Help on execute]
\? SqlExecute.execute

//@ Help on help
sql.help('help');

//@ Help on help, \? [USE:Help on help]
\? SqlExecute.help

//@ Help on sql
sql.help('sql');

//@ Help on sql, \? [USE:Help on sql]
\? SqlExecute.sql

//@ Finalization
mySession.close();

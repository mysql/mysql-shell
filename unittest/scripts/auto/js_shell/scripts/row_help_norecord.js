//@ Initialization
var mySession = mysqlx.getSession(__uripwd + '/mysql');
var res = mySession.sql("select 1").execute();
var row = res.fetchOne();

//@ Row help
row.help();

//@ Row help, \? [USE:Row help]
\? Row

//@ Help on length
row.help('length');

//@ Help on length, \? [USE:Help on length]
\? Row.length

//@ Help on getField
row.help('getField');

//@ Help on getField, \? [USE:Help on getField]
\? Row.getField

//@ Help on getLength
row.help('getLength');

//@ Help on getLength, \? [USE:Help on getLength]
\? Row.getLength

//@ Help on help
row.help('help');

//@ Help on help, \? [USE:Help on help]
\? Row.help

//@ Finalization
mySession.close();

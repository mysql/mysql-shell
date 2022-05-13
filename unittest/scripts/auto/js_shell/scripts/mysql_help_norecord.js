//@ mysql help
mysql.help()

//@ mysql help, \? [USE:mysql help]
\? mysql

//@ getClassicSession help
mysql.help('getClassicSession')

//@ getClassicSession help, \? [USE:getClassicSession help]
\? getClassicSession

//@ getSession help
mysql.help('getSession')

//@ getSession help, \? [USE:getSession help]
\? mysql.getSession

//@ mysql help on help
mysql.help('help')

//@ mysql help on help, \? [USE:mysql help on help]
\? mysql.help

//@ parseStatementAst
\? mysql.parseStatementAst

//@ quoteIdentifier
\? mysql.quoteIdentifier

//@ splitScript
\? mysql.splitScript

//@ unquoteIdentifier
\? mysql.unquoteIdentifier

shell.connect(__uripwd + '/mysql')
sqlexecute = session.sql('select 1;')
session.close()

#@ sqlexecute
sqlexecute.help()

#@ global ? for SqlexEcute[USE:sqlexecute]
\? SqlExecute

#@ global help for SqlexEcute[USE:sqlexecute]
\help SqlexEcute

#@ sqlexecute.bind
sqlexecute.help('bind')

#@ global ? for bind[USE:sqlexecute.bind]
\? SqlExecute.bind

#@ global help for bind[USE:sqlexecute.bind]
\help SqlExecute.bind

#@ sqlexecute.execute
sqlexecute.help('execute')

#@ global ? for execute[USE:sqlexecute.execute]
\? SqlExecute.execute

#@ global help for execute[USE:sqlexecute.execute]
\help SqlExecute.execute

#@ sqlexecute.help
sqlexecute.help('help')

#@ global ? for help[USE:sqlexecute.help]
\? SqlExecute.help

#@ global help for help[USE:sqlexecute.help]
\help SqlExecute.help

#@ sqlexecute.sql
sqlexecute.help('sql')

#@ global ? for sql[USE:sqlexecute.sql]
\? SqlExecute.sql

#@ global help for sql[USE:sqlexecute.sql]
\help SqlExecute.sql

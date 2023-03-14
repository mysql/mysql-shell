shell.connect(__mysqluripwd)
session.close()

#@ session
session.help()

#@ global ? for Session[USE:session]
\? ClassicSession

#@ global help for Session[USE:session]
\help ClassicSession

#@ session.close
session.help('close')

#@ global ? for close[USE:session.close]
\? ClassicSession.close

#@ global help for close[USE:session.close]
\help ClassicSession.close

#@ session.commit
session.help('commit')

#@ global ? for commit[USE:session.commit]
\? ClassicSession.commit

#@ global help for commit[USE:session.commit]
\help ClassicSession.commit

#@ session.get_uri
session.help('get_uri')

#@ global ? for get_uri[USE:session.get_uri]
\? ClassicSession.get_uri

#@ global help for get_uri[USE:session.get_uri]
\help ClassicSession.get_uri

#@ session.help
session.help('help')

#@ global ? for help[USE:session.help]
\? ClassicSession.help

#@ global help for help[USE:session.help]
\help ClassicSession.help

#@ session.is_open
session.help('is_open')

#@ global ? for is_open[USE:session.is_open]
\? ClassicSession.is_open

#@ global help for is_open[USE:session.is_open]
\help ClassicSession.is_open

#@ session.query
session.help('query')

#@ global ? for query[USE:session.query]
\? ClassicSession.query

#@ global help for query[USE:session.query]
\help ClassicSession.query

#@ session.rollback
session.help('rollback')

#@ global ? for rollback[USE:session.rollback]
\? ClassicSession.rollback

#@ global help for rollback[USE:session.rollback]
\help ClassicSession.rollback

#@ session.run_sql
session.help('run_sql')

#@ global ? for run_sql[USE:session.run_sql]
\? ClassicSession.run_sql

#@ global help for run_sql[USE:session.run_sql]
\help ClassicSession.run_sql

#@ session.start_transaction
session.help('start_transaction')

#@ global ? for start_transaction[USE:session.start_transaction]
\? ClassicSession.start_transaction

#@ global help for start_transaction[USE:session.start_transaction]
\help ClassicSession.start_transaction

#@ session.uri
session.help('uri')

#@ global ? for uri[USE:session.uri]
\? ClassicSession.uri

#@ global help for uri[USE:session.uri]
\help ClassicSession.uri
